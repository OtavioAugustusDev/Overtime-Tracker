CREATE DATABASE IF NOT EXISTS pineapple;
USE pineapple;

CREATE TABLE IF NOT EXISTS users (
    id INT PRIMARY KEY AUTO_INCREMENT,
    username VARCHAR(100) UNIQUE NOT NULL,
    password VARCHAR(255) NOT NULL,
    overtime_hours DECIMAL(10,2) DEFAULT 0.00,
    work_hours DECIMAL(10,2) DEFAULT 40.00,
    role ENUM('USER','GESTOR') DEFAULT 'USER',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS time_off_requests (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_id INT NOT NULL,
    date DATE NOT NULL,
    hours DECIMAL(10,2) NOT NULL,
    notes TEXT,
    status ENUM('PENDENTE', 'APROVADO', 'NEGADO') DEFAULT 'PENDENTE',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

CREATE INDEX idx_user_requests ON time_off_requests(user_id);
CREATE INDEX idx_status ON time_off_requests(status);

INSERT INTO users (username, password, overtime_hours, work_hours, role)
VALUES ('otavio', '1234', 0.00, 40.00, 'GESTOR');

INSERT INTO users (username, password, overtime_hours, work_hours, role)
VALUES ('breno', '1234', 25.50, 40.00, 'USER');
