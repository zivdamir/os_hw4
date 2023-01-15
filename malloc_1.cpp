#include <unistd.h>

void* smalloc(size_t size)
{
    if(size == 0 || size > 1e8)
	{
		return NULL;
	}
	
	void* before_malloc_program_break = sbrk(0);
	void* after_malloc_program_break = sbrk(size);
	
	if(after_malloc_program_break != before_malloc_program_break ||after_malloc_program_break=-1 ||before_malloc_program_break=-1)
	{
		return NULL;
	}
	
	return after_malloc_program_break;
}
