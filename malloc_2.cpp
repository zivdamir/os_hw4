#include <unistd.h>
#include <stdbool.h>
#include <cstring>
void sfree(void* p);
struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};

//need to initialize the list
MallocMetadata* block_list_head=NULL;
//a

void* initialize_list(size_t size)
{
    void* starting_block=sbrk(0);
    void* ptr_block_end=sbrk(sizeof(struct MallocMetadata)+size);
    if(ptr_block_end!=starting_block)
    {
        return NULL;
    }
    MallocMetadata* head_data=(MallocMetadata*)ptr_block_end;
    head_data->is_free= false;
    head_data->prev=NULL;
    head_data->size=size;
    head_data->next=NULL;
    //maximum, change
    block_list_head=head_data;

    return ptr_block_end;
}
void insert_metadata_sorted(MallocMetadata* node){

    MallocMetadata* head=block_list_head;
    //2 options, first its head, second not.
    //assume block_list_head isn't null
    if(head > node)
    {
        //this is the new head
        node->next=head;
        node->prev=NULL;
        head->prev=node;
        block_list_head=node;
        return;
    }
    MallocMetadata* temp=block_list_head;
    //else, not the head , look for it.

    while(temp->next!=NULL)
    {
        if(node<temp->next)
        {
            //insert to next and temp's next is our node.
            MallocMetadata* temp_next =temp->next;
            temp_next->prev=node;
            temp->next=node;
            node->prev = temp;
            node->next= temp_next;
            return;
        }
        temp=temp->next;
    }
    //in case node is bigger than all the elements in the list
    temp->next = node;
    node->next = NULL;
    return;
}
void* find_free_block(size_t size)
{
    MallocMetadata* temp=block_list_head;
    MallocMetadata* to_return=NULL;

    while(temp->next!=NULL&& temp->is_free==true )
    {
        if(temp->size >= size)
        {
            to_return=temp;
            break;
        }
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
    if(block_list_head == NULL)
    {
        initialize_list(size);
    }
    MallocMetadata* found = (MallocMetadata*)find_free_block(size);
    if(found == NULL)
    {
        void* starting_block = sbrk(0);
        void* ptr_block_end=sbrk(sizeof(struct MallocMetadata)+size);
        if(starting_block!=ptr_block_end)
        {
            return NULL;
        }
        MallocMetadata* data=(MallocMetadata*)ptr_block_end;
        data->is_free= false;
         data->prev = NULL;
        data->size = size;
        data->next = NULL;
        insert_metadata_sorted(data);
        return (data + sizeof(struct MallocMetadata));
    }
    found->is_free = false;
    return found + sizeof(struct MallocMetadata);
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
MallocMetadata* old_block_meta_data = (MallocMetadata*)((char*)oldp-sizeof(struct MallocMetadata));
if(size <= old_block_meta_data->size)
{
    old_block_meta_data->size = size;
    return oldp;
}
else
{
    char* newp  = (char*)smalloc(size);
    if(newp == NULL)
    {
        return NULL;
    }
    memmove(newp,oldp,old_block_meta_data->size);
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
    MallocMetadata* ptr_to_metadata = (MallocMetadata*)((char*)p-sizeof(struct MallocMetadata));
    ptr_to_metadata->is_free = true;
    return;
}
size_t _num_free_blocks(){
    int count = 0;
    MallocMetadata* head = block_list_head;
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
                count+= 1;
            }
            head = head->next;
        }
    }
    return count;
}
size_t _num_free_bytes();
size_t _num_allocated_blocks(){
    int count = 0;
    MallocMetadata* head = block_list_head;
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

    return 0;
}
size_t _num_meta_data_bytes(){
    return 0;
}
size_t _size_meta_data(){
    return 0;
}
