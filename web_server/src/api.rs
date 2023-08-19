use actix_web::{get, Responder, HttpResponse, web};
use std::ptr;
extern crate libc;

use libc::c_void;
use crate::shm;
// mod senser;


#[get("/")]
pub async fn api333() -> impl Responder {
    HttpResponse::Ok().body("api test")
}

#[get("/sensor")]
async fn sensor_data() -> impl Responder {
    println!("sensor start");
    let (shm_fd, shm_ptr) = unsafe { shm::init_shm() };

    let read_sensor_data: shm::ShmSensor = unsafe { ptr::read(shm_ptr as *const shm::ShmSensor) };

    println!("Read data from shared memory - Temp: {}, Press: {}, Humidity: {}",
        read_sensor_data.temp,
        read_sensor_data.press,
        read_sensor_data.humidity
    );

    shm::clean_shm(shm_fd);
    HttpResponse::Ok().body("senser")
}

pub fn config(cfg: &mut web::ServiceConfig) {
    cfg.service(
        web::scope("/api")
            .service(api333)
            .service(sensor_data)
    );
}