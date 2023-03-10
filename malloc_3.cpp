#include <unistd.h>
#include <stdbool.h>
#include <cstring>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <cassert>
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
void remove_from_list(MallocData data);
bool merge_if_possible(MallocData data);
void print_list(MallocData head);
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
void divide_and_insert(void* address_of_mallocdata, size_t starting_block_size, size_t second_block_size){
    MallocData first_block = (MallocData)address_of_mallocdata;
    check_for_valid_cookie_value(first_block);
    first_block->size = starting_block_size;
    first_block->is_free=false;
    first_block->next=NULL;
    first_block->prev=NULL;
    first_block->cookies_rand = random_value_for_cookie;
    first_block->is_mmap = false;
    MallocData second_block = (MallocData) ((long)(address_of_mallocdata)+starting_block_size+_size_meta_data());
    second_block->size=second_block_size;
    second_block->cookies_rand = random_value_for_cookie;
    second_block->next = NULL;
    second_block->prev=NULL;
    second_block->is_free=true;
    second_block->is_mmap = false;


    if(!merge_if_possible(second_block))
    {
        insert_metadata_sorted(second_block);
    }

    insert_metadata_sorted(first_block);
    



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
    //zivzivziv
    
    //printf("in malloc now \n");
    //print_list(block_list_head_sbrk);
    if (size >= KB * MIN_BLOCK_SIZE_TO_KEEP)
    {
        //use mmap
       // printf("wtf \n");
        void *allocated_mmap = mmap(NULL, size + _size_meta_data(), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE,
                                    -1, 0);
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
   // printf("line 273 %ld \n",size);
    if (block_list_head_sbrk == NULL)
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
        //printf("line 291 %ld \n",size);
        MallocData last_in_list = find_last_in_list();
        check_for_valid_cookie_value(last_in_list);
        /**levi changed**/
        MallocData data = NULL;
        if(last_in_list->is_free==true)
        {
            remove_from_list(last_in_list);
            sbrk(-(last_in_list->size + _size_meta_data()));
        }
        /**levi changed**/

        data = (MallocData) initialize_node(size);
        check_for_valid_cookie_value(data);
        if ( data == NULL)
        {
            return NULL;
        }
//hello
        data->is_free=false;
       // printf("before insert %ld\n",size);
        insert_metadata_sorted(data);
       // printf("after insert %ld\n",size);
        return (void *)((long)data + sizeof(struct MallocMetadata));
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





2. After the process described in the previous section, if one of the options ???a??? to ???d??? worked,
and the unused section of the block is large enough, split the block (according to the
instructions in challenge 1)!
3. You can assume that we will not test cases where we will reallocate an mmap() allocated
block to be resized to a block (excluding the meta-data) that???s less than 128kb.
4. You can assume that we will not test cases where we will reallocate a normally allocated
block to be resized to a block (excluding the meta-data) that???s more than 128kb.
5. When srealloc() is called on an mmaped block, you are never to re-use previous blocks,
meaning that a new block must be allocated (unless old_size==new_size).
.*/
void* srealloc(void* oldp, size_t size){
 //   printf(" 366 sbrk is %d\n", (long)sbrk(0)%100000);
    if(size==0 || size > 1e8)
    {
        return NULL;
    }
    if(oldp==NULL)
    {
       // printf("how dafuq oldp is null now \n");
        return smalloc(size);
    }

    MallocData block = (MallocData)((long)oldp-_size_meta_data());
    
    check_for_valid_cookie_value(block);
    //printf("levi im going to punch you if you dont shut up,np \n");
    if(block->is_mmap==false) {
       // printf(" 381 sbrk is %d\n", (long)sbrk(0)%100000);
        
        MallocData closest_block_behind = find_closest_block_behind(block);
       // printf("384 sbrk is %d\n", (long)sbrk(0)%100000);
        
       //print_list(block_list_head_sbrk);
        MallocData closest_block_in_front = find_closest_block_in_front(block);
      //  printf("levi im going to punch you if you dont shut up,np \n");
        
        size_t block_size =  block->size;
        //can be NULLS  CHECK IT OUT BEFORE
        size_t block_behind_size_with_meta = 0;
        size_t block_in_front_size_with_meta = 0 ;
        if(closest_block_behind!=NULL)
        {
        block_behind_size_with_meta = closest_block_behind->size + _size_meta_data();
        }
         if(closest_block_in_front!=NULL)
        {
        block_in_front_size_with_meta = closest_block_in_front->size + _size_meta_data();
        }
        /**conditions**/

        bool can_use_block_behind = (closest_block_behind != NULL && closest_block_behind->is_free == true);
        bool the_block_behind_is_big_enough = (size <= block_size + block_behind_size_with_meta);

        bool block_is_wilderness = ((block) == find_last_in_list());

        bool can_use_block_in_front = (closest_block_in_front != NULL && closest_block_in_front->is_free == true);
       // bool the_block_in_front_is_big_enough = (size <= block_size + block_in_front_size_with_meta);
        bool the_block_in_front_is_big_enough = (size <= block_size + block_in_front_size_with_meta);
        bool three_blocks_are_enough = (size <= block_behind_size_with_meta + block_in_front_size_with_meta + block_size);
        //  printf("levi im going to punch you if you dont shut up,np \n");
        bool block_in_front_is_wilderness = (closest_block_in_front == find_last_in_list());
        /**conditions**/
       // printf("block validation at 403 \n");
      // printf("levi im going to punch you if you dont shut up,np \n");
        check_for_valid_cookie_value(block);
        //MallocMetadata* old_block_meta_data = (MallocMetadata*)((long)oldp-sizeof(struct MallocMetadata));

        /**a. Try to reuse the current block without any merging.**/
        if (size <= block_size) {
       // printf("case a \n");
        //  printf("levi im going to punch you if you dont shut up,np \n");
        // levi, we dont always divide and insert.. only when we have enough size
        check_for_valid_cookie_value(block);
        if (block_size >= MIN_BLOCK_SIZE_TO_KEEP + _size_meta_data() + size)
        {
            remove_from_list(block);
            divide_and_insert(block, size, (block->size - _size_meta_data() - size));
            
            }
            //else, dont do anything, we dont need to!
            return oldp;
        }
        /**b. Try to merge with the adjacent block with the lower address.
        ??? If the block is the wilderness chunk, enlarge it after merging if needed.**/
        if (can_use_block_behind &&(the_block_behind_is_big_enough||(!the_block_behind_is_big_enough && block_is_wilderness))) {
            remove_from_list(block);
            remove_from_list(closest_block_behind);
            check_for_valid_cookie_value(block);
            check_for_valid_cookie_value(closest_block_behind);
            
         //   printf("closest behind size: %d, prev: %d, next: %d\n",closest_block_behind->size,closest_block_behind->prev,closest_block_behind->next);
            
            MallocData merged_block = merge_blocks_into_block_one(closest_block_behind, block);
            
            //printf(" merged_block size: %d, prev: %d, next: %d\n",merged_block->size,merged_block->prev,merged_block->next);

            check_for_valid_cookie_value(merged_block);
            merged_block->is_free = false;
            merged_block->is_mmap = false;
            void *newp = (void *)((long)merged_block + _size_meta_data());
            // change here, we dont always divide and insert, we have a conditon for that
            memmove(newp, oldp, block_size);//block_size is oldp.
            if(merged_block->size >= size+MIN_BLOCK_SIZE_TO_KEEP+_size_meta_data())
            {
               // printf("size_1: %d, size_2: %d\n", size, (merged_block->size - _size_meta_data() - size));
                divide_and_insert(merged_block, size, (merged_block->size - _size_meta_data() - size));
                return newp;
            }
            else {
            if (!the_block_behind_is_big_enough && block_is_wilderness) {
            size_t delta_to_sbrk = size - merged_block->size;
            sbrk(delta_to_sbrk); // check if null
            merged_block->size = merged_block->size + delta_to_sbrk;
            check_for_valid_cookie_value(merged_block);
            }
            insert_metadata_sorted(merged_block);
            return newp;
            }
/**??? If the block is the wilderness chunk, enlarge it after merging if needed.**/
            // if (!the_block_behind_is_big_enough && block_is_wilderness) {
            // size_t delta_to_sbrk = size - merged_block->size;
            // sbrk(delta_to_sbrk); // check if null
            // merged_block->size = merged_block->size + delta_to_sbrk;
            // check_for_valid_cookie_value(merged_block);
            // }
            }
        /**c. If the block is the wilderness chunk, enlarge it.**/
        if (block_is_wilderness) {
            //fixes-maybe we dont need to enlarge it .. check it.. todo
            if(block_size<=size)
            {
            size_t delta_to_sbrk = size - block->size;
            sbrk(delta_to_sbrk); //todo check if null
            block->size = block->size + delta_to_sbrk;
            check_for_valid_cookie_value(block);
            }
            else{
                //block size is greater.
                if(block_size >= _size_meta_data()+MIN_BLOCK_SIZE_TO_KEEP+size)
                {
                remove_from_list(block);
                divide_and_insert(block, size, block_size - _size_meta_data() - size);
                check_for_valid_cookie_value(block);
                }
                //else block size isn't enoguh, we dont have anything to do with it.
            }
            block->is_free = false;
            return (void *)((long)block + _size_meta_data());
        }

        /**d. Try to merge with the adjacent block with the higher address.**/
        if (can_use_block_in_front&&the_block_in_front_is_big_enough) {
          //  printf("case d \n");
            remove_from_list(closest_block_in_front);
            remove_from_list(block);

            check_for_valid_cookie_value(block);
            check_for_valid_cookie_value(closest_block_in_front);

            MallocData merged_block = merge_blocks_into_block_one(block, closest_block_in_front);
            check_for_valid_cookie_value(merged_block);
            if(merged_block->size>=size+MIN_BLOCK_SIZE_TO_KEEP+_size_meta_data())
            {
                //we can split it after..
                divide_and_insert(merged_block, size, merged_block->size - _size_meta_data() - size);
            }
            else
            {
            insert_metadata_sorted(merged_block);
            }
            merged_block->is_free = false;
            return (void *)((long)merged_block + _size_meta_data());
            // maybe we don't need to merge it
            // check_for_valid_cookie_value(merged_block);
        }

        /**e. Try to merge all those three adjacent blocks together.**/
        if (can_use_block_behind && can_use_block_in_front) {
            if (three_blocks_are_enough) {
            
           // printf("case e \n");
            
            remove_from_list(closest_block_in_front);
            remove_from_list(closest_block_behind);
            remove_from_list(block);

            check_for_valid_cookie_value(block);
            check_for_valid_cookie_value(closest_block_in_front);
            check_for_valid_cookie_value(closest_block_behind);

            MallocData merged_block = merge_blocks_into_block_one(block, closest_block_in_front);
            check_for_valid_cookie_value(merged_block);
            //MallocData merged_block2 = merge_blocks_into_block_one(merged_block, closest_block_behind);
            merged_block = merge_blocks_into_block_one(merged_block, closest_block_behind);
            check_for_valid_cookie_value(merged_block);
            
            if(merged_block->size >= size+MIN_BLOCK_SIZE_TO_KEEP+_size_meta_data())
            {
                divide_and_insert(merged_block, size, (merged_block->size - _size_meta_data() - size));
               
            }
            else{
                insert_metadata_sorted(merged_block);
            }
            merged_block->is_free = false;
            merged_block->is_mmap = false;
            void *newp = (void *)((long)merged_block + _size_meta_data());
            memmove(newp, oldp, size);
            return (void *)((long)merged_block + _size_meta_data());
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
                merged_block->is_free = false;
                void* newp=(void *)((long)merged_block + sizeof(struct MallocMetadata));
                memmove(newp, oldp, size);
                return newp;
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
                merged_block->is_free = false;
                void *newp = (void *)((long)merged_block + _size_meta_data());
                memmove(newp, oldp, size);
                return newp;
            }
        }
            /**g. Try to find a different block that???s large enough to contain the request (don???t forget
            that you need to free the current block, therefore you should, if possible, merge it
            with neighboring blocks before proceeding).**/
        //smalloc(size);

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
   // printf("line 675 \n");
   // check_for_valid_cookie_value(iterator);
    //printf("line 677 \n");
    /*if(block==iterator) // IN CASE where the head is actually him
    {
        return NULL;
    }*/
    while(iterator!=NULL)
    {
        check_for_valid_cookie_value(iterator);
        //printf("678 \n");
        if(iterator < block)
        {
            //iterator+_size_meta_data+iterator->size==block.
            //printf("680 \n");
            if(closest_block_behind == NULL || closest_block_behind < iterator)
            {
                closest_block_behind = iterator;
            }
        //    printf("684 \n");
        }
     //   printf("686 \n");
        iterator = iterator->next;
        // printf("688 \n");
      //  printf("%x\n", iterator);
        

        //printf("%x\n", block);
        //printf("%x\n", iterator->next);
        // zivziv here
    }
    //big problem ere
    if(closest_block_behind!=NULL)
    {
    check_for_valid_cookie_value(closest_block_behind);
    }
   // printf("returning \n");
    return closest_block_behind;
}
MallocData find_closest_block_in_front(MallocData block)
{
    MallocData closest_block_in_front=NULL;
    MallocData iterator = block_list_head_sbrk;
    
    
 
    while(iterator!=NULL)
    {
        check_for_valid_cookie_value(iterator);
        if(iterator > block)
        {
            
            if(closest_block_in_front == NULL || closest_block_in_front > iterator)
            {
                closest_block_in_front = iterator;
            }
          
        }
        iterator = iterator->next;
    }
    //big problem ere
    if(closest_block_in_front!=NULL)
    {
    check_for_valid_cookie_value(closest_block_in_front);
    }
   // printf("returning \n");
    return closest_block_in_front;
}


/**levi added**/
bool merge_if_possible(MallocData block)
{
    //printf("707 \n");
    MallocData closest_block_behind=NULL;
   // printf("710 \n");
    closest_block_behind = find_closest_block_behind(block);
   // printf("711\n");
    if(closest_block_behind!=NULL)
    {
       // printf("not null ,checking for cookies 737 \n");
        check_for_valid_cookie_value(closest_block_behind);
      //  printf("not null ,checking for cookies 739 \n");
    }
    MallocData closest_block_in_front = NULL;
   //  printf("find_closest_block_in_front \n");
    closest_block_in_front = find_closest_block_in_front(block);
   // printf("found \n");
    if (closest_block_in_front != NULL)
    {
        //
      //  printf("not null ,checking for cookies 745 \n");
    check_for_valid_cookie_value(closest_block_in_front);
  //   printf("not null ,checking for cookies 747 \n");
    }
    /*zivs addition*/
    if(closest_block_behind!=NULL && closest_block_behind->is_free==true &&closest_block_in_front!=NULL && closest_block_in_front->is_free==true)
    {
         //printf("front and closest_block_behind line 716  \n");
       //  printf("  in_fron and behind case \n");
        remove_from_list(block);
        remove_from_list(closest_block_behind);
        remove_from_list(closest_block_in_front);
        MallocData merged_prev_and_curr = merge_blocks_into_block_one(closest_block_behind, block);
        check_for_valid_cookie_value(merged_prev_and_curr);
        MallocData merged_with_next = merge_blocks_into_block_one(merged_prev_and_curr, closest_block_in_front);
        check_for_valid_cookie_value(merged_with_next);
        insert_metadata_sorted(merged_with_next);
        return true;
    }
    /*zivs addition*/
    else/*zivs addition*/
    {
    if(closest_block_behind!=NULL && closest_block_behind->is_free==true)
    {
     //   printf(" only in behind case \n");
        //printf(" closest_block_behind line 732  \n");
        remove_from_list(block);
        remove_from_list(closest_block_behind);
        MallocData merged=merge_blocks_into_block_one(block,closest_block_behind);
        check_for_valid_cookie_value(merged);
        insert_metadata_sorted(merged);
        return true;/*zivs addition*/
    }
    if(closest_block_in_front!=NULL && closest_block_in_front->is_free==true)
    {
       // printf("front_block_behind line 742  \n");
      //  printf(" only in front case \n");
        remove_from_list(block);
        remove_from_list(closest_block_in_front);
        MallocData merged=merge_blocks_into_block_one(block,closest_block_in_front);
        check_for_valid_cookie_value(merged);
       
        insert_metadata_sorted(merged);
       
        return true; /*zivs addition*/
    }
    //in case where nothing happens,(we haven't found anything to merge..)
  //  printf("nothing of the above(behind&&front) happend(merge_if possible)\n");
    return false; /*zivs addition*/
    }
}
/**levi added**/

void sfree(void* p){
    //assume that its a pointer to the actual data.
    //printf("line 755  \n");
    if(p == NULL)
    {
        return;
    }
    
  // printf("line 761 \n");
    MallocData ptr_to_metadata = (MallocData)((long)p-sizeof(struct MallocMetadata));
   // printf("line 763  \n");
    check_for_valid_cookie_value(ptr_to_metadata);
  // printf("line 765  \n");
    if (ptr_to_metadata->is_mmap == false)
    {
   // printf("line 768  \n");
    ptr_to_metadata->is_free = true;//changed the location to be above merge_if_possible,I think its better that way, maybe it wasn't
    //your intention, let me know. anyway we merge it with a free block behind it and one block after it so we can always set it
    //there to is_free=true;
    merge_if_possible(ptr_to_metadata); //return it!
  //printf("line 793\n");
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
     //  printf("head_Sbrk null \n");
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
      //  printf("head_Sbrk null \n");
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
       // printf("its null\n");
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
       // printf("block_head_mmap_is null \n");
        return count;
    }
    else
    {
        while(head!=NULL)
        {
           //printf("block_head_mmap_isnt null \n");
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
       // printf("head_Sbrk null \n");
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
