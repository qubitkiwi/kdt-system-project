use actix_files::NamedFile;
use actix_web::{web};

pub fn config(cfg: &mut web::ServiceConfig) {
    cfg.service(
        web::resource("/").route(web::get().to(index))
    );
}

async fn index() -> Result<NamedFile, actix_web::Error> {
    Ok(NamedFile::open("./build/index.html")?)
}
