#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/errno.h>

int sc_restrict(int syscall_num)
{
	int i, j;
	scr* curr_service;
	int restriction_threshold, proc_restriction_level, log_size;

	task_t* wanted_task = current;
	// not happen anyway
	if (wanted_task == NULL)
	{
		return -1;
	}
	// we don't check on idle (0) and init (1) processes
	if(wanted_task->pid == 0 || wanted_task->pid == 1)
	{
		return 0;
	}
	
	log_size = wanted_task->log_size;
	// find a restriction for our process contains the given syscall
	for (i = 0; i < wanted_task->restrictions_list_size; i++)
	{	
		curr_service = &(wanted_task->restrictions_list[i]);
		
		if (curr_service->syscall_num == syscall_num)
		{
			restriction_threshold = curr_service->restriction_threshold;
			proc_restriction_level = wanted_task->proc_restriction_level;
			
			if (proc_restriction_level < restriction_threshold)
			{
				// shifting - in case of new log (log_size = 0), the shift is not needed and hence the for-loop is not entered.
				for (j = log_size - 1; j >= 0; j--)
				{
					// if last-cell is beyond the array area - so skip.
					if (j + 1 == 100)
					{
						continue;
					}
					
					(wanted_task->log[j+1]).syscall_num = (wanted_task->log[j]).syscall_num;
					(wanted_task->log[j+1]).syscall_restriction_threshold = (wanted_task->log[j]).syscall_restriction_threshold;
					(wanted_task->log[j+1]).proc_restriction_level = (wanted_task->log[j]).proc_restriction_level;
					(wanted_task->log[j+1]).time = (wanted_task->log[j]).time;
				}

				// limitation of 100 fai-s per log
				if (log_size < 100)
				{
					log_size++;
				}
				// update in task_struct
				wanted_task->log_size = log_size;
				
				// update a new 1st fai
				(wanted_task->log[0]).syscall_num = syscall_num;
				(wanted_task->log[0]).syscall_restriction_threshold = restriction_threshold;
				(wanted_task->log[0]).proc_restriction_level = proc_restriction_level;
				(wanted_task->log[0]).time = jiffies;
				
				return -1;
			}
			
			break;
		}
	}
	return 0;
}
