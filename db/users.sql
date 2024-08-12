CREATE TYPE UserStatus AS ENUM ('ACTIVE', 'INACTIVE', 'BLOCKED');

CREATE TYPE UserPlatform AS ENUM ('TEST', 'STEAM');

CREATE TABLE users (
    id INT PRIMARY KEY AUTO_INCREMENT,
    user_name VARCHAR(30) CHECK (LENGTH(user_name) BETWEEN 6 AND 30),
    platform UserPlatform NOT NULL,
    status UserStatus NOT NULL,
    created TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

INSERT INTO users (user_name, platform, status) VALUES
('leesin', 'TEST', 'ACTIVE');
