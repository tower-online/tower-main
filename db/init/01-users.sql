CREATE TABLE users
(
    id       INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(30) UNIQUE NOT NULL,
    platform VARCHAR(8)         NOT NULL CHECK (platform IN ('TEST', 'STEAM')),
    status   VARCHAR(8)         NOT NULL CHECK (status IN ('ACTIVE', 'INACTIVE', 'BLOCKED')),
    created  TIMESTAMP          NOT NULL DEFAULT CURRENT_TIMESTAMP,

    CONSTRAINT check_username CHECK (username REGEXP '^[a-zA-Z0-9_]{6,30}$')
);

CREATE TABLE user_stats
(
    user_id INT,

    karma   INT NOT NULL DEFAULT 0,

    PRIMARY KEY (user_id),
    FOREIGN KEY (user_id) REFERENCES users (id)
);

INSERT INTO users (username, platform, status)
VALUES ('dummy_00001', 'TEST', 'ACTIVE');
