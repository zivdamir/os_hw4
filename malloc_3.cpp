#include <unistd.h>
#include <stdbool.h>
#include <cstring>
#define KB 1024
#define MIN_BLOCK_SIZE_TO_KEEP 128
size_t _size_meta_data();
/*Challegne 5:
Consider the following case – a buffer overflow happens in the heap memory area (either on
accident or on purpose), and this overflow overwrites the metadata bytes of some allocation
with arbitrary junk (or worse). Think – which problems can happen if we access this
overwritten metadata?
Solution: We can detect (but not prevent) heap overflows using “cookies” – 32bit integers
that are placed in the metadata of each allocation. If an overflow happens, the cookie value
will change and we can detect this before accessing the allocation’s metadata by comparing
the cookie value with the expected cookie value.
You are required to add cookies to the allocations’ metadata.
Note that cookie values should be randomized – otherwise they could be maliciously
overwritten with that same constant value to avoid overwrite detection. You can choose a
global random value for all the cookies used in the same process.
Change your current implementation, such that before every metadata access, you should
check if the relevant metadata cookie has changed. In case of overwrite detection, you
should immediately call exit(0xdeadbeef), as the process memory is corrupted and it cannot
continue (not recommended in practice).
Things to consider –
1. When should you choose the random value?
2. Where should the cookie be placed in the metadata? (most buffer overflows
happen from lower addresses to higher addresses)
Note: You are not requested to “narrow” down the heap anywhere in this section. The only
exception for allowing free memory to go back to the system is in challenge 4, when using
munmap().
Note: As opposed to the previous section, the ‘size’ field in the metadata for blocks here changes.
Notes about srealloc():
srealloc() requires some complicated edge-case treatment now. Use the following guidelines:
1. If srealloc() is called on a block and you find that this block and one of or both
neighboring blocks are large enough to contain the request, merge and use them. Prioritize
as follows:
a. Try to reuse the current block without any merging.
b. Try to merge with the adjacent block with the lower address.
• If the block is the wilderness chunk, enlarge it after merging if needed.
c. If the block is the wilderness chunk, enlarge it.
d. Try to merge with the adjacent block with the higher address.
e. Try to merge all those three adjacent blocks together.
f. If the wilderness chunk is the adjacent block with the higher address: 
i. Try to merge with the lower and upper blocks (such as in e), and enlarge the
wilderness block as needed.
ii. Try to merge only with higher address (the wilderness chunk), and enlarge it as
needed.
g. Try to find a different block that’s large enough to contain the request (don’t forget
that you need to free the current block, therefore you should, if possible, merge it
with neighboring blocks before proceeding).
h. Allocate a new block with sbrk().
2. After the process described in the previous section, if one of the options ‘a’ to ‘d’ worked,
and the unused section of the block is large enough, split the block (according to the
instructions in challenge 1)!
3. You can assume that we will not test cases where we will reallocate an mmap() allocated
block to be resized to a block (excluding the meta-data) that’s less than 128kb.
4. You can assume that we will not test cases where we will reallocate a normally allocated
block to be resized to a block (excluding the meta-data) that’s more than 128kb.
5. When srealloc() is called on an mmaped block, you are never to re-use previous blocks,
meaning that a new block must be allocated (unless old_size==new_size).
Notes about mmap():
1. It is recommended to have another list for mmap() allocated blocks, separate from the list
of other allocations.
2. To find whether the block was allocated with mmap() or regularly, you can either add a new
field to the meta-data, or simply check if the ‘size’ field is greater than 128kb or not.
3. Remember to add support for your debug functions (function 5-10). Note that functions 5-6
should not consider munmapp()’ed areas as free.*/
void sfree(void* p);
typedef struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
}*MallocData;
void divide_and_insert(void* address, size_t starting_block_size, size_t second_block_size);
void insert_metadata_sorted(MallocData node);
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
void divide_and_insert(void* address, size_t starting_block_size, size_t second_block_size){
    MallocData first_block = (MallocData)address;
    first_block->size=starting_block_size;
    first_block->is_free=false;
    first_block->next=NULL;
    first_block->prev=NULL;
    insert_metadata_sorted(first_block);
    MallocData second_block = (MallocData) ((long)(address)+starting_block_size+_size_meta_data());
    second_block->size=second_block_size;
    second_block->next=NULL;
    second_block->prev=NULL;
    second_block->is_free=true;
    insert_metadata_sorted(second_block);
}
void insert_metadata_sorted(MallocData node){

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

/*levi helper function 0,0 */
void remove_from_list(MallocData block)
{
    MallocData prev_block = block->prev;
    MallocData next_block = block->next;
    if(prev_block!=NULL)
    {
        prev_block->next=next_block;
    }
    if(next_block!=NULL)
    {
        next_block->prev=prev_block;
    }
    if(prev_block == NULL)
    {
        block_list_head = next_block;
    }
    block->next=NULL;
    block->prev=NULL;
    return;
}

//need to implement a search for free block
//need to implement a sorted insertion

/**levi added**/
// assumes that list in now empty
MallocData find_last_in_list()
{
    MallocData last_in_list = block_list_head;
    MallocData iterator = block_list_head;
    while(iterator != NULL)
    {
        if(last_in_list<iterator)
        {
            last_in_list=iterator;
        }
    }
    return last_in_list;
}
/**levi added**/

void* smalloc(size_t size){
    if(size == 0 || size > 1e8)
    {
        return NULL;
    }
    //  printf("%x is our size \n",size);
    //printf("%d is data->size, while %d is size \n",data->size,size);
    if(block_list_head == NULL)
    {
        MallocData head = (MallocData)initialize_node(size);
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
        MallocData last_in_list = find_last_in_list();
        /**levi changed**/
        MallocData data = NULL;
        if(last_in_list->is_free==true)
        {
            sbrk(-(last_in_list->size+_size_meta_data()));
        }
        /**levi changed**/

        data = (MallocData) initialize_node(size);
        if ( data == NULL)
        {
            return NULL;
        }

        data->is_free=false;
        insert_metadata_sorted(data);

        return (void*)((long)data + sizeof(struct MallocMetadata));
    }
    /*changes challenge 1 are here*/
    else
    {
        if(found->size>=size+MIN_BLOCK_SIZE_TO_KEEP+_size_meta_data())
        {
            remove_from_list(found);
            divide_and_insert(found,size,(found->size-_size_meta_data()-size));

        }
        found->is_free = false;
        return (void*)((long)found + sizeof(MallocMetadata));
    }
    
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
/*done */



/*Challenge 2 (Memory utilization):
Many allocations and de-allocations might cause two adjacent blocks to be free, but
separate.
Solution: Implement a function that sfree() will use, such that if one adjacent block (next
or previous) was free, the function will automatically combine both free blocks (the current
one and the adjacent one) into one large free block. On the corner case where both the next
and previous blocks are free, you should combine all 3 of them into one large block. */
/*done*/



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

/**levi added**/
MallocData merge_blocks_into_block_one(MallocData block_one,MallocData block_two)/*ziv added*/
{
    MallocData merged_block=NULL;
    size_t second_block_size = 0;
    if(block_one<block_two)
    {
        merged_block = block_one;
        second_block_size = block_two->size;
    }
    else
    {
        merged_block = block_two;
        second_block_size = block_one->size;
    }

    merged_block->size = merged_block->size + second_block_size + _size_meta_data();
    block_one=merged_block; //maybe need to revert
    return merged_block; /*ziv added*/
}
/**levi added**/

/**levi added**/
void merge_if_possible(MallocData block)
{
    MallocData closest_block_behind=NULL;
    MallocData iterator = block_list_head;
    while(iterator!=NULL)
    {
        if(iterator < block)
        {
            if(closest_block_behind == NULL || closest_block_behind < iterator)
            {
                closest_block_behind = iterator;
            }
        }
        iterator = iterator->next;
    }

    MallocData closest_block_in_front = NULL;
    MallocData end_of_block = (MallocData) ((long) block  + block->size + _size_meta_data());// added ';', changed size to block->size
    if(end_of_block<sbrk(0))//changed from sbrk(NULL) to sbrk(0) for code coherency.
    {
        closest_block_in_front = end_of_block;
    }
   
    /*zivs addition*/
    if(closest_block_behind!=NULL && closest_block_behind->is_free==true &&closest_block_in_front!=NULL && closest_block_in_front->is_free==true)
    {
        remove_from_list(block);
        remove_from_list(closest_block_behind);
        remove_from_list(closest_block_in_front);
        MallocData merged_prev_and_curr = merge_blocks_into_block_one(closest_block_behind, block);
        MallocData merged_with_next = merge_blocks_into_block_one(merged_prev_and_curr, closest_block_in_front);
        insert_metadata_sorted(merged_with_next);
        return;
    }
    /*zivs addition*/
    else/*zivs addition*/
    {
    if(closest_block_behind!=NULL && closest_block_behind->is_free==true)
    {
        remove_from_list(block);
        remove_from_list(closest_block_behind);
        MallocData merged=merge_blocks_into_block_one(block,closest_block_behind);
        insert_metadata_sorted(merged);
        return;/*zivs addition*/
    }
    if(closest_block_in_front!=NULL && closest_block_in_front->is_free==true)
    {
        remove_from_list(block);
        remove_from_list(closest_block_in_front);
        MallocData merged=merge_blocks_into_block_one(block,closest_block_in_front);
        insert_metadata_sorted(merged);
        return;/*zivs addition*/
    }
    //in case where nothing happens,(we haven't found anything to merge..)
    return;/*zivs addition*/
    }
}
/**levi added**/

void sfree(void* p){
    //assume that its a pointer to the actual data.
    if(p == NULL)
    {
        return;
    }
    //ptr_to_metadata->is_free = true;
    MallocData ptr_to_metadata = (MallocData)((long)p-sizeof(struct MallocMetadata));
    ptr_to_metadata->is_free = true;//changed the location to be above merge_if_possible,I think its better that way, maybe it wasn't
    //your intention, let me know. anyway we merge it with a free block behind it and one block after it so we can always set it
    //there to is_free=true;
    merge_if_possible(ptr_to_metadata);
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
