use actix_web::{get, Responder, HttpResponse, web};
use std::ptr;

mod shm;


#[get("/")]
pub async fn api() -> impl Responder {
    HttpResponse::Ok().body("api test")
}

#[get("/bmp280")]
async fn bmp280_data() -> impl Responder {

    let (shm_fd, shm_ptr) = shm::init_shm();
    let bmp280_data: shm::ShmSensor = unsafe { ptr::read(shm_ptr as *const shm::ShmSensor) };

    // println!("Read data from shared memory - Temp: {}, Press: {}",
    //     bmp280_data.temp,
    //     bmp280_data.press
    // );

    HttpResponse::Ok().json(bmp280_data)
}

pub fn config(cfg: &mut web::ServiceConfig) {
    cfg.service(
        web::scope("/api")
            .service(api)
            .service(bmp280_data)
    );
}