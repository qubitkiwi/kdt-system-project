use libc::{c_int, c_void, O_RDWR, O_CREAT, shm_open, ftruncate, mmap, PROT_READ, MAP_SHARED, close, shm_unlink};
use std::{ptr, ffi::CString, mem};

// 공유 메모리 구조체 정의
#[repr(C)]
pub struct ShmSensor {
    pub temp: c_int,
    pub press: c_int,
    pub humidity: c_int
}

// const SHM_NAME: CString = CString::new("/my_shared_memory").expect("CString::new failed");
const SHM_NAME: &str = "/my_shared_memory";

pub fn init_shm() -> (c_int, *mut c_void) {
    
    let shm_fd: c_int = unsafe {
        shm_open(
            CString::new(SHM_NAME).expect("CString::new failed").as_ptr(),
            O_RDWR | O_CREAT,
            0o644,
        )
    };

    if shm_fd == -1 {
        panic!("Failed to open or create shared memory");
    }

    let shm_size = mem::size_of::<ShmSensor>() as i64;
    if unsafe { ftruncate(shm_fd, shm_size) } == -1 {
        panic!("Failed to set shared memory size");
    }

    let shm_ptr: *mut c_void = unsafe {
        mmap(
            ptr::null_mut(),
            shm_size as usize,
            PROT_READ,
            MAP_SHARED,
            shm_fd,
            0,
        ) as *mut c_void
    };

    if shm_ptr == libc::MAP_FAILED as *mut c_void {
        panic!("Failed to map shared memory");
    }

    (shm_fd, shm_ptr)
}

pub fn clean_shm(shm_fd: c_int) {
    unsafe {
        close(shm_fd);
        shm_unlink(CString::new(SHM_NAME).expect("CString::new failed").as_ptr());
    }
}