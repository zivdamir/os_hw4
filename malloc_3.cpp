#include <unistd.h>
#include <stdbool.h>
#include <cstring>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#define KB 1024
#define MIN_BLOCK_SIZE_TO_KEEP 128
size_t _size_meta_data();

int random_value_for_cookie = rand();
void sfree(void *p);
typedef struct MallocMetadata {
    int cookies_rand;
    size_t size;
    bool is_free;
    bool is_mmap;
    MallocMetadata *next;
    MallocMetadata* prev;
}*MallocData;
MallocData merge_blocks_into_block_one(MallocData block_one,MallocData block_two);
void check_for_valid_cookie_value(MallocData block){
    if (block == NULL)
    {
        return;
    }
    else
    {
        if(block->cookies_rand!=random_value_for_cookie)
            {
                exit(0xdeadbeef);
            }
    }
    return;
}
MallocData find_closest_block_behind(MallocData block);
MallocData find_closest_block_in_front(MallocData block);

void divide_and_insert(void *address, size_t starting_block_size, size_t second_block_size);
void insert_metadata_sorted(MallocData node);
//need to initialize the list
MallocData block_list_head_sbrk=NULL;
//a
MallocData block_list_head_mmap = NULL;
void *initialize_node(size_t size)
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
    head_data->cookies_rand = random_value_for_cookie;
    head_data->is_free = true;
    head_data->prev=NULL;
    head_data->size=size;
    head_data->is_mmap = false;
    // printf("init list %d is data->size, while %d is size \n",head_data->size,size);
    head_data->next=NULL;
    //maximum, change
    //block_list_head_sbrk=head_data;

    return ptr_block_end;
}
void divide_and_insert(void* address, size_t starting_block_size, size_t second_block_size){
    MallocData first_block = (MallocData)address;
    check_for_valid_cookie_value(first_block);
    first_block->size = starting_block_size;
    first_block->is_free=false;
    first_block->next=NULL;
    first_block->prev=NULL;
    insert_metadata_sorted(first_block);
    MallocData second_block = (MallocData) ((long)(address)+starting_block_size+_size_meta_data());
    second_block->size=second_block_size;
    second_block->cookies_rand = random_value_for_cookie;
    second_block->next = NULL;
    second_block->prev=NULL;
    second_block->is_free=true;
    insert_metadata_sorted(second_block);
}
void insert_metadata_sorted(MallocData node){

    MallocData head=block_list_head_sbrk;
    //2 options, first its head, second not.
    //assume block_list_head_sbrk isn't null
     if(head == NULL)
    {
        block_list_head_sbrk=node;
        return;
    }
    if(node->size < head->size)
    {
        //this is the new head
        node->next=head;
        node->prev=NULL;
        head->prev=node;
        block_list_head_sbrk=node;
        return;
    }
    //else, not the head , look for it.

    while(head->next!=NULL)
    {
        if(node->size<=head->next->size)
        {
            if(node->size!=head->next->size) {
                //insert to next and temp's next is our node.
                MallocMetadata *temp_next = head->next;
                check_for_valid_cookie_value(temp_next);
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
                check_for_valid_cookie_value(temp_next);
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
    MallocData temp=block_list_head_sbrk;
    MallocData to_return=NULL;
    check_for_valid_cookie_value(temp);
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
    check_for_valid_cookie_value(prev_block);
    MallocData next_block = block->next;
    check_for_valid_cookie_value(next_block);
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
        if(block->is_mmap==false)
        {

        block_list_head_sbrk = next_block;

        }
        else{

        block_list_head_mmap = next_block;

        }
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
    MallocData last_in_list = block_list_head_sbrk;
    MallocData iterator = block_list_head_sbrk;
    check_for_valid_cookie_value(iterator);
    while (iterator != NULL)
    {
        if(last_in_list<iterator)
        {
            last_in_list=iterator;
        }
        iterator = iterator->next;
        check_for_valid_cookie_value(iterator);
    }
    return last_in_list;
}
/**levi added**/
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
void* smalloc(size_t size){
    if(size == 0 || size > 1e8)
    {
        return NULL;
    }
    if (size >= KB*MIN_BLOCK_SIZE_TO_KEEP)
    {
        //use mmap
        void* allocated_mmap= mmap(NULL , size+_size_meta_data() , PROT_READ | PROT_WRITE , MAP_ANONYMOUS |MAP_PRIVATE,
                  -1 , 0);
        if(allocated_mmap==MAP_FAILED)
        {
            return NULL;
        }
        MallocData mmap_meta_data = (MallocData)allocated_mmap;
        mmap_meta_data->next = NULL;
        mmap_meta_data->prev = NULL;
        mmap_meta_data->is_free = false;
        mmap_meta_data->is_mmap = true;
        mmap_meta_data->size = size;
        mmap_meta_data->cookies_rand = random_value_for_cookie;
        if (block_list_head_mmap == NULL)
        {
            block_list_head_mmap = mmap_meta_data;
        } // make it head
        else
        {
            MallocData prev_head = block_list_head_mmap;
            check_for_valid_cookie_value(prev_head);
            prev_head->prev = mmap_meta_data;
            mmap_meta_data->next = prev_head;
            block_list_head_mmap = mmap_meta_data;
        }
        return (void*)((long)allocated_mmap+_size_meta_data());
        //todo , add here support for
    }

    if(block_list_head_sbrk == NULL)
    {
        MallocData head = (MallocData)initialize_node(size);
        check_for_valid_cookie_value(head);
        if (head == NULL)
        {
            return NULL;
        }
        insert_metadata_sorted(head);
        block_list_head_sbrk=head;

    }
    MallocData found = (MallocData)find_free_block(size);
    check_for_valid_cookie_value(found);
    if (found == NULL)
    {
        MallocData last_in_list = find_last_in_list();
        check_for_valid_cookie_value(last_in_list);
        /**levi changed**/
        MallocData data = NULL;
        if(last_in_list->is_free==true)
        {
            sbrk(-(last_in_list->size+_size_meta_data()));
        }
        /**levi changed**/

        data = (MallocData) initialize_node(size);
        check_for_valid_cookie_value(data);
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
    
    if (allocated == NULL)
    {
        return NULL;
    }
    check_for_valid_cookie_value((MallocData)((long)(allocated) - _size_meta_data()));
    memset(allocated,0,size*num);
    return allocated;
}
/*
Notes about srealloc():
srealloc() requires some complicated edge-case treatment now. Use the following guidelines:
1. If srealloc() is called on a block and you find that this block and one of or both
neighboring blocks are large enough to contain the request, merge and use them. Prioritize
as follows:





2. After the process described in the previous section, if one of the options ‘a’ to ‘d’ worked,
and the unused section of the block is large enough, split the block (according to the
instructions in challenge 1)!
3. You can assume that we will not test cases where we will reallocate an mmap() allocated
block to be resized to a block (excluding the meta-data) that’s less than 128kb.
4. You can assume that we will not test cases where we will reallocate a normally allocated
block to be resized to a block (excluding the meta-data) that’s more than 128kb.
5. When srealloc() is called on an mmaped block, you are never to re-use previous blocks,
meaning that a new block must be allocated (unless old_size==new_size).
.*/
void* srealloc(void* oldp, size_t size){
    if(size==0 || size > 1e8)
    {
        return NULL;
    }
    if(oldp==NULL)
    {
        return smalloc(size);
    }

    MallocData block = (MallocData)((long)oldp-_size_meta_data());
    check_for_valid_cookie_value(block);
    if(block->is_mmap==false) {
        MallocData closest_block_behind = find_closest_block_behind(block);
        MallocData closest_block_in_front = find_closest_block_behind(block);
        
        size_t block_size =  block->size;
        //can be NULLS  CHECK IT OUT BEFORE
        size_t block_behind_size_with_meta = closest_block_behind->size + _size_meta_data();
        size_t block_in_front_size_with_meta = closest_block_in_front->size + _size_meta_data();

        /**conditions**/

        bool can_use_block_behind = (closest_block_behind != NULL && closest_block_behind->is_free == true);
        bool the_block_behind_is_big_enough = (size <= block_size + block_behind_size_with_meta);

        bool block_is_wilderness = ((block) == find_last_in_list());

        bool can_use_block_in_front = (closest_block_in_front != NULL && closest_block_in_front->is_free == true);
       // bool the_block_in_front_is_big_enough = (size <= block_size + block_in_front_size_with_meta);

        bool three_blocks_are_enough = (size <= block_behind_size_with_meta + block_in_front_size_with_meta + block_size);

        bool block_in_front_is_wilderness = (closest_block_in_front == find_last_in_list());
        /**conditions**/

        check_for_valid_cookie_value(block);
        //MallocMetadata* old_block_meta_data = (MallocMetadata*)((long)oldp-sizeof(struct MallocMetadata));

        /**a. Try to reuse the current block without any merging.**/
        if (size <= block_size) {
            
            //levi, we dont always divide and insert.. only when we have enough size
            check_for_valid_cookie_value(block);
            if(block_size>=MIN_BLOCK_SIZE_TO_KEEP+_size_meta_data()+size)
            {
            remove_from_list(block);
            divide_and_insert(block, size, (block->size - _size_meta_data() - size));
            }
            //else, dont do anything, we dont need to!
            return oldp;
        }
        /**b. Try to merge with the adjacent block with the lower address.
        • If the block is the wilderness chunk, enlarge it after merging if needed.**/
        if (can_use_block_behind) {
            bool was_expanded = false;

            /**• If the block is the wilderness chunk, enlarge it after merging if needed.**/
            if (!the_block_behind_is_big_enough && block_is_wilderness) {
                size_t delta_to_sbrk = size - block_behind_size_with_meta - block_size;
                sbrk(delta_to_sbrk);//check if null
                block->size = block->size + delta_to_sbrk;
                check_for_valid_cookie_value(block);
                was_expanded = true;
            }
            if (was_expanded || the_block_behind_is_big_enough) {
                remove_from_list(block);
                remove_from_list(closest_block_behind);
                check_for_valid_cookie_value(block);
                check_for_valid_cookie_value(closest_block_behind);

                MallocData merged_block = merge_blocks_into_block_one(closest_block_behind, block);
                check_for_valid_cookie_value(merged_block);
                merged_block->is_free = false;
                //change here, we dont always divide and insert, we have a conditon for that
                divide_and_insert(merged_block, size, (block->size - _size_meta_data() - size));
                check_for_valid_cookie_value(merged_block);
                //this is incorrect , not 
                return (void *) ((long) merged_block + sizeof(struct MallocMetadata));
            }
        }
        /**c. If the block is the wilderness chunk, enlarge it.**/
        if (block_is_wilderness) {
            //fixes-maybe we dont need to enlarge it .. check it.. todo
            size_t delta_to_sbrk = size - block->size;
            sbrk(delta_to_sbrk); //todo check if null
            block->size = block->size + delta_to_sbrk;
            check_for_valid_cookie_value(block);
            return (void *)((long)block + sizeof(struct MallocMetadata));
        }

        /**d. Try to merge with the adjacent block with the higher address.**/
        if (can_use_block_in_front) {
            remove_from_list(closest_block_in_front);
            remove_from_list(block);

            check_for_valid_cookie_value(block);
            check_for_valid_cookie_value(closest_block_in_front);

            MallocData merged_block = merge_blocks_into_block_one(block, closest_block_in_front);

            check_for_valid_cookie_value(merged_block);
            //maybe we don't need to merge it
            divide_and_insert(merged_block, size, (block->size - _size_meta_data() - size));
            check_for_valid_cookie_value(merged_block);

            return (void *) ((long) merged_block + sizeof(struct MallocMetadata));
        }

        /**e. Try to merge all those three adjacent blocks together.**/
        if (can_use_block_behind && can_use_block_in_front) {
            if (three_blocks_are_enough) {
                remove_from_list(closest_block_in_front);
                remove_from_list(closest_block_behind);
                remove_from_list(block);

                check_for_valid_cookie_value(block);
                check_for_valid_cookie_value(closest_block_in_front);
                check_for_valid_cookie_value(closest_block_behind);

                MallocData merged_block = merge_blocks_into_block_one(block, closest_block_in_front);
                check_for_valid_cookie_value(merged_block);

                merged_block = merge_blocks_into_block_one(merged_block, closest_block_behind);
                check_for_valid_cookie_value(merged_block);

                insert_metadata_sorted(merged_block);
                check_for_valid_cookie_value(merged_block);

                return (void *) ((long) merged_block + sizeof(struct MallocMetadata));
            }
        }
        /**f. If the wilderness chunk is the adjacent block with the higher address: **/
        if (block_in_front_is_wilderness && can_use_block_in_front) {
            /**i. Try to merge with the lower and upper blocks (such as in e), and enlarge the
                wilderness block as needed.**/
            if (can_use_block_behind) {
                size_t delta_to_sbrk =
                        size - (block_behind_size_with_meta + block_size + block_in_front_size_with_meta);
                sbrk(delta_to_sbrk); //todo check if null
                block->size = block->size + delta_to_sbrk;
                check_for_valid_cookie_value(block);

                remove_from_list(closest_block_in_front);
                remove_from_list(closest_block_behind);
                remove_from_list(block);

                check_for_valid_cookie_value(block);
                check_for_valid_cookie_value(closest_block_in_front);
                check_for_valid_cookie_value(closest_block_behind);

                MallocData merged_block = merge_blocks_into_block_one(block, closest_block_in_front);
                check_for_valid_cookie_value(merged_block);

                merged_block = merge_blocks_into_block_one(merged_block, closest_block_behind);
                check_for_valid_cookie_value(merged_block);

                insert_metadata_sorted(merged_block);
                check_for_valid_cookie_value(merged_block);

                return (void *) ((long) merged_block + sizeof(struct MallocMetadata));
            }
                /**ii. Try to merge only with higher address (the wilderness chunk), and enlarge it as
                        needed.**/
            else {
                size_t delta_to_sbrk = size - (block_size + block_in_front_size_with_meta);
                sbrk(delta_to_sbrk); //todo check if null
                block->size = closest_block_in_front->size + delta_to_sbrk;
                check_for_valid_cookie_value(closest_block_in_front);

                remove_from_list(closest_block_in_front);
                remove_from_list(block);

                check_for_valid_cookie_value(block);
                check_for_valid_cookie_value(closest_block_in_front);

                MallocData merged_block = merge_blocks_into_block_one(block, closest_block_in_front);

                check_for_valid_cookie_value(merged_block);
                insert_metadata_sorted(merged_block);
                check_for_valid_cookie_value(merged_block);

                return (void *) ((long) merged_block + _size_meta_data());
            }
        }
            /**g. Try to find a different block that’s large enough to contain the request (don’t forget
            that you need to free the current block, therefore you should, if possible, merge it
            with neighboring blocks before proceeding).**/


            /**h. Allocate a new block with sbrk().**/

        else {
            void *newp = smalloc(size);
            if (newp == NULL) {
                return NULL;
            }
            memmove(newp, oldp, block_size);
            sfree(oldp);
            return newp;
        }
    }
    else
    {
        //is mapped->true
        if(block->size==size)
        {
            return oldp;
        }
        else
        {
            
            //int status=munmap(block,block->size+_size_meta_data());
           
            void* new_block=mmap(NULL,_size_meta_data()+size,PROT_WRITE|PROT_READ,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
            MallocData mmap_meta_data = (MallocData)new_block;
            mmap_meta_data->next = NULL;
            mmap_meta_data->prev = NULL;
            mmap_meta_data->is_free = false;
            mmap_meta_data->is_mmap = true;
            mmap_meta_data->size = size;
            mmap_meta_data->cookies_rand = random_value_for_cookie;
            if (block_list_head_mmap == NULL)
            {
                block_list_head_mmap = mmap_meta_data;
            } // make it head
            
            else
            {
                MallocData prev_head = block_list_head_mmap;
                check_for_valid_cookie_value(prev_head);
                prev_head->prev = mmap_meta_data;
                mmap_meta_data->next = prev_head;
                block_list_head_mmap = mmap_meta_data;
            }
            memmove((void*)((long)new_block+_size_meta_data()),oldp,block->size);
            return (void*)((long)new_block+_size_meta_data());
            //check tommorow to see if its still compiling, good night levi.
            //~fu~,you meant good night ziv.
            //right? ich will


        }
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
        check_for_valid_cookie_value(block_one);
        second_block_size = block_two->size;
    }
    else
    {
        merged_block = block_two;
        check_for_valid_cookie_value(block_two);
        second_block_size = block_one->size;
    }

    merged_block->size = merged_block->size + second_block_size + _size_meta_data();
    check_for_valid_cookie_value(merged_block);
    block_one = merged_block; // maybe need to revert
    return merged_block; /*ziv added*/
}
/**levi added**/

MallocData find_closest_block_behind(MallocData block)
{
    MallocData closest_block_behind=NULL;
    MallocData iterator = block_list_head_sbrk;
    check_for_valid_cookie_value(iterator);
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
        check_for_valid_cookie_value(iterator);
    }
    check_for_valid_cookie_value(closest_block_behind);
    return closest_block_behind;
}
MallocData find_closest_block_in_front(MallocData block)
{
    MallocData closest_block_in_front = NULL;
    MallocData end_of_block = (MallocData) ((long) block  + block->size + _size_meta_data());// added ';', changed size to block->size
    if(end_of_block<sbrk(0))//changed from sbrk(NULL) to sbrk(0) for code coherency.
    {
        closest_block_in_front = end_of_block;
        check_for_valid_cookie_value(closest_block_in_front);
    }
    check_for_valid_cookie_value(closest_block_in_front);
    return closest_block_in_front;
}

/**levi added**/
void merge_if_possible(MallocData block)
{
    MallocData closest_block_behind=NULL;
    closest_block_behind = find_closest_block_behind(block);
    check_for_valid_cookie_value(closest_block_behind);
    MallocData closest_block_in_front = NULL;
    closest_block_in_front = find_closest_block_in_front(block);
    check_for_valid_cookie_value(closest_block_in_front);
    /*zivs addition*/
    if(closest_block_behind!=NULL && closest_block_behind->is_free==true &&closest_block_in_front!=NULL && closest_block_in_front->is_free==true)
    {
        remove_from_list(block);
        remove_from_list(closest_block_behind);
        remove_from_list(closest_block_in_front);
        MallocData merged_prev_and_curr = merge_blocks_into_block_one(closest_block_behind, block);
        check_for_valid_cookie_value(merged_prev_and_curr);
        MallocData merged_with_next = merge_blocks_into_block_one(merged_prev_and_curr, closest_block_in_front);
        check_for_valid_cookie_value(merged_with_next);
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
        check_for_valid_cookie_value(merged);
        insert_metadata_sorted(merged);
        return;/*zivs addition*/
    }
    if(closest_block_in_front!=NULL && closest_block_in_front->is_free==true)
    {
        remove_from_list(block);
        remove_from_list(closest_block_in_front);
        MallocData merged=merge_blocks_into_block_one(block,closest_block_in_front);
        check_for_valid_cookie_value(merged);
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
    check_for_valid_cookie_value(ptr_to_metadata);
    if (ptr_to_metadata->is_mmap == false)
    {
    ptr_to_metadata->is_free = true;//changed the location to be above merge_if_possible,I think its better that way, maybe it wasn't
    //your intention, let me know. anyway we merge it with a free block behind it and one block after it so we can always set it
    //there to is_free=true;
    merge_if_possible(ptr_to_metadata);
    }
    if(ptr_to_metadata->is_mmap==true)
    {
    remove_from_list(ptr_to_metadata);
    munmap(ptr_to_metadata, ptr_to_metadata->size + _size_meta_data());
    }
    return;
}

size_t _num_free_blocks(){
    int count = 0;
    MallocData head = block_list_head_sbrk;
    check_for_valid_cookie_value(head);
    if (head == NULL)
    {
        //  printf("HEAD IS NULL \n");
        return count;
    }
    else
    {
        MallocData temp = block_list_head_sbrk;
        check_for_valid_cookie_value(temp);
        while(temp!=NULL)
        {
            // printf("debugging num_free_blocks\n");
            if(temp->is_free == true)
            {
                count+= 1;
            }
            temp = temp->next;
            check_for_valid_cookie_value(temp);
        }
    }
    return count;
}
size_t _num_free_bytes(){
    int count = 0;
    MallocData head = block_list_head_sbrk;
    check_for_valid_cookie_value(head);
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
            check_for_valid_cookie_value(head);
        }
    }
    return count;
}
size_t _num_allocated_blocks(){
    int count = 0;
    MallocData head = block_list_head_sbrk;
    check_for_valid_cookie_value(head);
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
            check_for_valid_cookie_value(head);
        }
    }
    head = block_list_head_mmap;
    check_for_valid_cookie_value(head);
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
            check_for_valid_cookie_value(head);
        }
    }
    return count;
}
size_t _num_allocated_bytes()
{

    size_t count = 0;
    MallocData head = block_list_head_sbrk;
    check_for_valid_cookie_value(head);
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
            check_for_valid_cookie_value(head);
        }
    }
    head = block_list_head_mmap;
    check_for_valid_cookie_value(head);
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
            check_for_valid_cookie_value(head);
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
