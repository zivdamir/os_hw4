#include <unistd.h>
#include <stdbool.h>
#include <cstring>

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
    /*if(node < head)
    {
        //this is the new head
        node->next=head;
        node->prev=NULL;
        head->prev=node;
        block_list_head=node;
        return;
    }*/
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
