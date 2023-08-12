use actix_web::{get, Responder, HttpResponse};

#[get("/api")]
async fn api() -> impl Responder {
    HttpResponse::Ok().body("api test")
}
