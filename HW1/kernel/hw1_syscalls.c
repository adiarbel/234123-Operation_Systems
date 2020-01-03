#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/slab.h>

//#include <sys/types.h>


int sys_sc_restrict(pid_t pid, int proc_restriction_level, scr* restrictions_list, int list_size)
{
	int ret;
	task_t* wanted_task = find_task_by_pid(pid);
	scr* kernel_list = NULL;
	fai* log_curr_list = NULL;
	
	
	if (pid < 0) 
	{
		return -1*ESRCH;
	}
	//Could use a pid=0/1 condition
	else if(wanted_task==NULL)
	{
		return -1*ESRCH;
	}
	else if (proc_restriction_level < 0 || proc_restriction_level > 2) 
	{
		return -1*EINVAL;
	}
	
	else if (list_size < 0) 
	{
		return -1*EINVAL;
	}

	
	if(list_size>0)
	{
		// Upon first restrict
		if(wanted_task->log==NULL)		
		{
			log_curr_list = (fai*)kmalloc(100 * sizeof(fai),GFP_KERNEL);
			if (log_curr_list == NULL)
			{
				return -1*ENOMEM;
			}
		}
		
		// Create scr list for copying the list
		kernel_list = (scr*)kmalloc(list_size * sizeof(scr),GFP_KERNEL);
		if (kernel_list == NULL)
		{
			return -1*ENOMEM;
		}
		
		// copy from user
		if (restrictions_list == NULL)
		{
			kfree(log_curr_list);
			kfree(kernel_list);
			return -1*ENOMEM;
		}
		ret = copy_from_user(kernel_list, restrictions_list, list_size * sizeof(scr));
		if (ret > 0)
		{
			kfree(log_curr_list);
			kfree(kernel_list);
			return -1*ENOMEM;
		}

	}	
	
	if(wanted_task->restrictions_list != NULL)
	{
		kfree(wanted_task->restrictions_list);
	}
	
	wanted_task->proc_restriction_level = proc_restriction_level;
	
	wanted_task->restrictions_list = kernel_list;
	
	wanted_task->restrictions_list_size = list_size;	
	
	
	// Upon first restrict
	if(wanted_task->log==NULL)		
	{
		wanted_task->log = log_curr_list;
		
		wanted_task->log_size = 0;
	}
	// return for success or other errors!!!!!!!!!!
	return 0;
}

int sys_set_proc_restriction ( pid_t pid , int proc_restriction_level)
{
	
	task_t* wanted_task;
	
	if (pid < 0) 
	{
		return -1*ESRCH;
	}
	
	wanted_task = find_task_by_pid(pid);
	
	if(wanted_task==NULL)
	{
		return -1*ESRCH;
	}
	else if (proc_restriction_level < 0 || proc_restriction_level > 2) 
	{
		return -1*EINVAL;
	}
	else if (pid < 2) 
	{
		return -1*EINVAL;
	}
	
	wanted_task->proc_restriction_level = proc_restriction_level;
	
	
	// return for success or other errors!!!!!!!!!!
	
	return 0;
}

int sys_get_process_log(pid_t pid, int size, fai* user_mem)
{
	int ret;
	int i;
	task_t* wanted_task = find_task_by_pid(pid);
	fai* user_log ;
	if (pid < 0) 
	{
		return -1*ESRCH;
	}	
	else if(wanted_task==NULL)
	{
		return -1*ESRCH;
	}
	else if (wanted_task->log_size < size) 
	{
		return -1*EINVAL;
	}
	else if (size<0) 
	{
		return -1*EINVAL;
	}
	//User memory filling
	else if(size==0)
	{
		return 0;
	}
	//Allocation of reversing array
	user_log = (fai*)kmalloc(size * sizeof(fai),GFP_KERNEL);
	if(user_log == NULL)
	{
		return -1*ENOMEM;
	}
	//Reversing
	for(i=size-1;i>=0;i--)
	{
		user_log[i] = wanted_task->log[size-i-1];
	}
	//Copying to user memory space
	ret = copy_to_user(user_mem, user_log, size * sizeof(fai));
	if (ret > 0)
	{
		kfree(user_log);
		return -1*ENOMEM;
	}
	//Freeing user log
	kfree(user_log);

	return 0;
}
