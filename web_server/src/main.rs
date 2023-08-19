use actix_web::{App, web, HttpServer, HttpResponse, Result};
mod shm;
mod api;
mod static_page;
// use libc::c_void;


async fn not_found() -> Result<HttpResponse> {
    Ok(HttpResponse::NotFound().body("404"))
}

#[actix_web::main]
async fn main() -> std::io::Result<()> {
    

    let state = HttpServer::new(|| {
        App::new()
            .configure(api::config)
            .configure(static_page::config)
            .default_service(web::route().to(not_found))
    })
    .bind(("127.0.0.1", 8080))?
    .run()
    .await;

    

    state
}