
use std::ffi::CString;
use nix::sys::stat::Mode;
use nix::mqueue::{mq_attr_member_t, mq_open, mq_close, mq_receive, mq_send, MQ_OFlag};

fn main() {
		const MSG_SIZE: mq_attr_member_t = 32;
    let mq_name= CString::new("/a_nix_test_queue").unwrap();
    
    let oflag0 = MQ_OFlag::O_CREAT | MQ_OFlag::O_WRONLY;
    let mode = Mode::S_IWUSR | Mode::S_IRUSR | Mode::S_IRGRP | Mode::S_IROTH;
    let mqd0 = mq_open(&mq_name, oflag0, mode, None).unwrap();
    let msg_to_send = b"msg_1";
    mq_send(&mqd0, msg_to_send, 1).unwrap();
    
    let oflag1 = MQ_OFlag::O_CREAT | MQ_OFlag::O_RDONLY;
    let mqd1 = mq_open(&mq_name, oflag1, mode, None).unwrap();
    let mut buf = [0u8; 32];
    let mut prio = 0u32;
    let len = mq_receive(&mqd1, &mut buf, &mut prio).unwrap();
    assert_eq!(prio, 1);
    assert_eq!(msg_to_send, &buf[0..len]);
    
    mq_close(mqd1).unwrap();
    mq_close(mqd0).unwrap();
}