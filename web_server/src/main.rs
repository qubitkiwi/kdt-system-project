use actix_web::{App, web, HttpServer, HttpResponse, Result};
mod api;
use actix_files;


async fn not_found() -> Result<HttpResponse> {
    Ok(HttpResponse::NotFound().body("404"))
}

#[actix_web::main]
async fn main() -> std::io::Result<()> {
    
    let state = HttpServer::new(|| {
        App::new()
            .configure(api::config)
            .service(actix_files::Files::new("/", "./dist/").index_file("index.html"))
            .default_service(web::route().to(not_found))
    })
    .bind(("0.0.0.0", 8080))?
    .run()
    .await;


    state
}