#include <linux/orientation.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>

void print_orientation(struct dev_orientation orient)
{
	printk("Azimuth: %d\n", orient.azimuth);
	printk("Pitch: %d\n", orient.pitch);
	printk("Roll: %d\n", orient.roll);
}

int orient_equals(struct dev_orientation one, struct dev_orientation two)
{
	return (one.azimuth == two.azimuth &&
			one.pitch == two.pitch &&
			one.roll == two.roll);
}

int range_equals(struct orientation_range *range,
		struct orientation_range *target)
{
	return (range->azimuth_range == target->azimuth_range &&
			range->pitch_range == target->pitch_range &&
			range->roll_range == target->roll_range &&
			orient_equals(range->orient, target->orieng));
}

int generic_search_list(struct orientation_range *target,
			struct list_head *list, int type)
{
	int flag = 1;
	struct list_head *current;
	list_for_each(current, list) {
		struct lock_entry *entry;
		if (list == &waiters_list)
			entry = list_entry(current, struct lock_entry, list);
		else
			entry = list_entry(current, struct lock_entry,
					granted_list);
		if (lock_entry->type == type &&
				range_equals(lock_entry->range, target)) {
			flag = 0;
			break;
		}
	}
	return flag;
}

int no_writer_waiting(struct orientation_range *target)
{
	return generic_search_list(target, &waiters_list, 1);
}

int no_writer_grabbed(struct orientation_range *target)
{
	return generic_search_list(target, &granted_list, 1);
}

int no_reader_grabbed(struct orientation_range *target)
{
	return generic_search_list(target, &granted_list, 0);
}

void grant_lock(struct lock_entry *entry)
{
	entry->granted = 1;
	list_add_tail(&granted_list, entry->granted_list);
	spin_lock(&WAITERS_LOCK);
	list_del(entry->list);
	spin_unlock(&WAITERS_UNLOCK);
}

int in_range(struct orientation_range *range, struct dev_orientation orient)
{
	struct dev_orientation basis = range->orient;
	if (orient.azimuth > basis.azimuth + range->azimuth_range
		|| orient.azimuth < basis.azimuth + range->azimuth_range)
		return 0;
	if (orient.pitch > basis.pitch + range->pitch_range
		|| orient.pitch < basis.pitch + range->pitch_range)
		return 0;
	if (orient.roll > basis.roll + range->roll_range
		|| orient.roll < basis.roll + range->roll_range)
		return 0;
	return 1;
}

void process_waiter(struct list_head *current)
{
	struct lock_entry *entry = list_entry(current, struct lock_entry, list);
	if (in_range(entry->range, current_orient)) {
		if (entry->type == 0)
			if (no_writer_waiting() && no_writer_grabbed())
				grant_lock(entry);
		else
			if (no_writer_grabbed() && no_reader_grabbed())
				grant_lock(entry);
	}
}

SYSCALL_DEFINE1(set_orientation, struct dev_orientation __user *, orient)
{
	//TODO: Lock set_orientation for multiprocessing
	if (copy_from_user(&current_orient, orient,
				sizeof(struct dev_orientation)) != 0)
		return -EFAULT;
	
	struct list_head *current;
	list_for_each(current, &waiters_list)
		process_waiter(current);

	print_orientation(current_orient);

	return 0;
}

SYSCALL_DEFINE1(orientlock_read, struct orientation_range __user *, orient)
{
	struct orientation_range *korient;
	struct lock_entry *entry;
	
	korient = kmalloc(sizeof(orientation_range), GFP_KERNEL);
	if (copy_from_user(korient, orient, sizeof(orientation_range)) != 0)
		return -EFAULT;

	entry = kmalloc(sizeof(entry), GFP_KERNEL);
	entry->range = korient;
	entry->granted = 0;
	INIT_LIST_HEAD(&entry->list);
	INIT_LIST_HEAD(&entry->granted_list);
	entry->type = 0;

	spin_lock(&WAITERS_LOCK);
	list_add_tail(&head, &waiters_list);
	spin_unlock(&WAITERS_LOCK);
	DEFINE_WAIT(wait);
	
	add_wait_queue(sleepers, &wait);
	while(!entry.granted) {
		prepare_to_wait(&sleepers, &wait, TASK_INTERRUPTIBLE);
		schedule();
	}
	finish_wait(&sleepers, &wait);
}

SYSCALL_DEFINE1(orientlock_write, struct orientation_range __user *, orient)
{
	struct orientation_range *korient;
	struct lock_entry *entry;
	
	korient = kmalloc(sizeof(orientation_range), GFP_KERNEL);
	if (copy_from_user(korient, orient, sizeof(orientation_range)) != 0)
		return -EFAULT;

	entry = kmalloc(sizeof(entry), GFP_KERNEL);
	entry->range = korient;
	entry->granted = 0;
	INIT_LIST_HEAD(&entry->list);
	INIT_LIST_HEAD(&entry->granted_list);
	entry->type = 1;
	
	spin_lock(&WAITERS_LOCK);
	list_add_tail(&head, &waiters_list);
	spin_unlock(&WAITERS_LOCK);

	DEFINE_WAIT(wait);
	
	add_wait_queue(sleepers, &wait);
	while(!entry.granted) {
		prepare_to_wait(&sleepers, &wait, TASK_INTERRUPTIBLE);
		schedule();
	}
	finish_wait(&sleepers, &wait);
}

SYSCALL_DEFINE1(orientunlock_read, struct orientation_range __user *, orient)
{
	struct orientation_range korient;
	if (copy_from_user(&korient, orient, sizeof(orientation_range)) != 0)
		return -EFAULT;
	
	struct list_head *current;
	struct lock_entry *entry;
	list_for_each(current, &granted_list) {
		entry = list_entry(current, struct lock_entry, granted_list);
		if (range_equals(korient, current->range) && entry->type == 0)
			break;
		else
			; //no locks with the orientation_range available
	}
	list_del(current);
	kfree(entry->range);
	kfree(entry);
}

SYSCALL_DEFINE1(orientunlock_write, struct orientation_range __user *, orient)
{
	//TODO: Remember to free lock_entry and range
}
