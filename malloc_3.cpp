#include <unistd.h>
#include <stdbool.h>
#include <cstring>
#define KB 1024
void sfree(void* p);
typedef struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
}*MallocData;

//need to initialize the list
MallocData block_list_head=NULL;
//a

void* initialize_node(size_t size)
{

    //printf("sbrk-ing \n");
    void* starting_block=sbrk(0);
    void* ptr_block_end=sbrk(sizeof(struct MallocMetadata)+size);
    if(starting_block!=ptr_block_end)
    {
        //*(long*)ptr_block_end==-1||
        return NULL;
    }
    MallocData head_data=(MallocData)ptr_block_end;
    head_data->is_free= true;
    head_data->prev=NULL;
    head_data->size=size;
    //printf("init list %d is data->size, while %d is size \n",head_data->size,size);
    head_data->next=NULL;
    //maximum, change
    //block_list_head=head_data;

    return ptr_block_end;
}
void insert_metadata_sorted(MallocMetadata* node){

    MallocData head=block_list_head;
    //2 options, first its head, second not.
    //assume block_list_head isn't null
    if(node->size < head->size)
    {
        //this is the new head
        node->next=head;
        node->prev=NULL;
        head->prev=node;
        block_list_head=node;
        return;
    }
    //else, not the head , look for it.
    if(head == NULL)
    {
        block_list_head=node;
        return;
    }
    while(head->next!=NULL)
    {
        if(node->size<=head->next->size)
        {
            if(node->size!=head->next->size) {
                //insert to next and temp's next is our node.
                MallocMetadata *temp_next = head->next;
                temp_next->prev = node;
                head->next = node;
                node->prev = head;
                node->next = temp_next;
                return;
            }
            else
            {
            if(node>head->next)
            {
                //we dont know if this is the only one with the same size as node ... as long as size is same keep going
                head=head->next;
                continue;
            }
            else
            {
                MallocMetadata *temp_next = head->next;
                temp_next->prev = node;
                head->next = node;
                node->prev = head;
                node->next = temp_next;
                return;
            }

            }
        }
        head=head->next;
    }
    //in case node is bigger than all the elements in the list
    head->next = node;
    node->prev=head;
    node->next = NULL;
    return;
}
void* find_free_block(size_t size)
{
    MallocData temp=block_list_head;
    MallocData to_return=NULL;

    while(temp!=NULL )
    {
        // printf("temp->size:= %d  , size:= %d\n",temp->size,size);
        if(temp->size >= size && temp->is_free==true)
        {
            // printf("found? :)\n");
            to_return=temp;
            break;
        }
        //printf("but its not free :( \n, although temp->is free is=%s \n",temp->is_free==true ? "true":"false");
        temp=temp->next;
    }
    return to_return;
}
//need to implement a search for free block
//need to implement a sorted insertion
void* smalloc(size_t size){
    if(size == 0 || size > 1e8)
    {
        return NULL;
    }
    //  printf("%x is our size \n",size);
    //printf("%d is data->size, while %d is size \n",data->size,size);
    if(block_list_head == NULL)
    {
        MallocData head=(MallocData)initialize_node(size);
        if(head==NULL)
        {
            return NULL;
        }
        insert_metadata_sorted(head);
        block_list_head=head;

    }
    MallocData found = (MallocData)find_free_block(size);
    if(found == NULL)
    {
        MallocData data=(MallocData) initialize_node(size);
        if ( data == NULL)
        {
            return NULL;
        }
        data->is_free=false;
        insert_metadata_sorted(data);

        return (void*)((long)data + sizeof(struct MallocMetadata));
    }
    // printf("found! \n");
    found->is_free = false;
    // printf("%x is data adress \n",found);
    //printf("im going to return something return is %ld,sizeof(MallocMetaData) is %ld,return found+sizeof(Mallocmetadata)) and its %ld \n",found,sizeof(MallocMetadata),(long)found+sizeof(MallocMetadata));
    return (void*)((long)found + sizeof(MallocMetadata));
    //look for free block
}
void* scalloc(size_t num, size_t size)
{

    void* allocated= smalloc(num*size);
    if(allocated == NULL)
    {
        return NULL;
    }
    memset(allocated,0,size*num);
    return allocated;
}
/*Challenge 1 (Memory utilization):
If we reuse freed memory sectors with bigger sizes than required, we’ll be wasting memory
(internal fragmentation).
Solution: Implement a function that smalloc() will use, such that if a pre-allocated block
is reused and is large enough, the function will cut the block into two smaller blocks with
two separate meta-data structs. One will serve the current allocation, and another will
remain unused for later (marked free and added to the list).
Definition of “large enough”: After splitting, the remaining block (the one that is not used)
has at least 128 bytes of free memory, excluding the size of your meta-data structure.
Note: Once again, you are not requested to find the “best” free block for this section, but
the first block that satisfies the allocation defined above*/

void split_into_two_objects(void* start_of_first_object,void* start_of_second_object);

/*Challenge 2 (Memory utilization):
Many allocations and de-allocations might cause two adjacent blocks to be free, but
separate.
Solution: Implement a function that sfree() will use, such that if one adjacent block (next
or previous) was free, the function will automatically combine both free blocks (the current
one and the adjacent one) into one large free block. On the corner case where both the next
and previous blocks are free, you should combine all 3 of them into one large block. */

void try_merge_freed_block(void* block);
//gets a free'd block , trying to merge from left and right.(go to prev and next if its possible, check if they're free,if yes merge, if no dont)


/*Challenge 3 (Memory utilization):
Define the “Wilderness” chunk as the topmost allocated chunk. Let’s presume this chunk is
free, and all others are full. It is possible that the new allocation requested is bigger than the
wilderness block, thus requiring us to call sbrk() once more – but now, it is easier to
simply enlarge the wilderness block, saving us an addition of a meta-data structure.
Solution: Change your current implementation, such that if:
1. A new request has arrived, and no free memory chunk was found big enough.
2. And the wilderness chunk is free.
Then enlarge the wilderness chunk enough to store the new request.  */
void enlarge_wilderness_block();
//goes to the wilderness block,enlarge it by sbrk-ing to the right size and then merge it using try_merge_freed_block() function.
bool check_for_wilderness();//returns true if wilderness condition applies, false otherwise.
/* Challenge 4 (Large allocations):
Recall from our first discussion that modern dynamic memory managers not only use
sbrk() but also mmap(). This process helps reduce the negative effects of memory
fragmentation when large blocks of memory are freed but locked by smaller, more recently
allocated blocks lying between them and the end of the allocated space. In this case, had the
block been allocated with sbrk(), it would have probably remained unused by the system
for some time (or at least most of it).
Solution: Change your current implementation, by looking up how you can use mmap()
and munmap() instead of sbrk() for your memory allocation unit. Use this only for
allocations that require 128kb space or more (128*1024 B).*/
//solution: in smalloc, make sure to check if size >= 128*1024 bytes., if yes , use mmap.
void* srealloc(void* oldp, size_t size){
    if(size==0 || size > 1e8)
    {
        return NULL;
    }
    if(oldp==NULL)
    {
        return smalloc(size);
    }
    size_t block_size = ((MallocData)((long)oldp - sizeof(MallocMetadata)))->size;

    //MallocMetadata* old_block_meta_data = (MallocMetadata*)((long)oldp-sizeof(struct MallocMetadata));
    if(size <= block_size)
    {
//old_block_meta_data->size = size;
        return oldp;
    }
    else
    {
        void* newp  = smalloc(size);
        if(newp == NULL)
        {
            return NULL;
        }
        memmove(newp,oldp,block_size);
        sfree(oldp);
        return newp;
    }

}
void sfree(void* p){
    //assume that its a pointer to the actual data.
    if(p == NULL)
    {
        return;
    }
    //ptr_to_metadata->is_free = true;
    MallocData ptr_to_metadata = (MallocData)((long)p-sizeof(struct MallocMetadata));

    ptr_to_metadata->is_free = true;
    return;
}

size_t _num_free_blocks(){
    int count = 0;
    MallocData head = block_list_head;
    if (head == NULL)
    {
        //  printf("HEAD IS NULL \n");
        return count;
    }
    else
    {
        MallocData temp = block_list_head;

        while(temp!=NULL)
        {
            // printf("debugging num_free_blocks\n");
            if(temp->is_free == true)
            {
                count+= 1;
            }
            temp = temp->next;
        }
    }
    return count;
}
size_t _num_free_bytes(){
    int count = 0;
    MallocData head = block_list_head;
    if (head == NULL)
    {
        return count;
    }
    else
    {
        while(head!=NULL)
        {
            if(head->is_free == true)
            {
                count+= head->size;
            }
            head = head->next;
        }
    }
    return count;
}
size_t _num_allocated_blocks(){
    int count = 0;
    MallocData head = block_list_head;
    if (head == NULL)
    {
        return count;
    }
    else
    {
        while(head!=NULL)
        {
            count+= 1;
            head = head->next;
        }
    }
    return count;
}
size_t _num_allocated_bytes()
{

    size_t count = 0;
    MallocData head = block_list_head;
    if (head == NULL)
    {
        return count;
    }
    else
    {
        while(head!=NULL)
        {
            count+= head->size;
            head = head->next;
        }
    }
    return count;
}
size_t _size_meta_data(){
    return sizeof(struct MallocMetadata);
}
size_t _num_meta_data_bytes(){
    return _size_meta_data()*_num_allocated_blocks();
}
