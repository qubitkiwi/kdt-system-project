mod api_handler;
mod static_page;
use actix_web::{App,HttpServer};


#[actix_web::main]
async fn main() -> std::io::Result<()> {
    HttpServer::new(|| {
        App::new()
            .service(api_handler::api)
            .configure(static_page::configure_routes)
    })
    .bind(("127.0.0.1", 8080))?
    .run()
    .await
}