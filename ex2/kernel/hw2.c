#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/sched.h>

asmlinkage long sys_hello(void) {
    printk("Hello, World!\n");
    return 0;
}

asmlinkage long sys_set_status(int status) {
    if(status!=0 && status!=1) {
        return -EINVAL;
    }

    current->faculty = status;
    return 0;
}

asmlinkage long sys_get_status(void) {
    return current->faculty;
}

asmlinkage long sys_register_process(void) {
    //Finding the init process
    struct task_struct* head = current;
    while(head->pid != 1) {
        head = head->real_parent;
    }

    if(list_empty(&head->recognized)) {
        INIT_LIST_HEAD(&head->recognized);
    }
    list_add(&current->recognized, &head->recognized);

    return 0;
}

asmlinkage long sys_get_all_cs(void) {
    //Finding the init process
    struct task_struct* head = current;
    while(head->pid != 1) {
        head = head->real_parent;
    }

    if(list_empty(&head->recognized)) {
        return -ENODATA;
    }

    //Adds the pids of the cs processes
    int pid_sum = 0;
    struct list_head* iter;
    list_for_each(iter, &head->recognized) {
        struct task_struct* curr = list_entry(iter, struct task_struct, recognized);
        if(curr->faculty == 1) {
            pid_sum += curr->pid;
        }
    }

    return pid_sum;
}