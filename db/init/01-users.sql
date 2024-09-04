CREATE TABLE users
(
    id        INT AUTO_INCREMENT PRIMARY KEY,
    username  VARCHAR(30) UNIQUE NOT NULL CHECK (username REGEXP '^[a-zA-Z0-9_]{6,30}$'),
    platform  VARCHAR(8)         NOT NULL CHECK (platform IN ('TEST', 'STEAM')),
    status    VARCHAR(8)         NOT NULL DEFAULT 'ACTIVE' CHECK (status IN ('ACTIVE', 'INACTIVE', 'BLOCKED')),
    privilege VARCHAR(8)         NULL CHECK (privilege IN (NULL, 'MANAGER', 'ADMIN')),
    created   TIMESTAMP          NOT NULL DEFAULT CURRENT_TIMESTAMP
);


CREATE TABLE user_stats
(
    user_id INT,

    karma   INT NOT NULL DEFAULT 0,

    PRIMARY KEY (user_id),
    FOREIGN KEY (user_id) REFERENCES users (id)
);

DELIMITER //
CREATE TRIGGER after_user_insert
    AFTER INSERT
    ON users
    FOR EACH ROW
BEGIN
    INSERT INTO user_stats (user_id)
    VALUES (NEW.id);
END //
DELIMITER ;


DELIMITER //
CREATE TRIGGER after_user_delete
    AFTER DELETE
    ON users
    FOR EACH ROW
BEGIN
    DELETE
    FROM user_stats
    WHERE user_id = OLD.id;

    DELETE
    FROM characters
    WHERE user_id = OLD.id;
END //
DELIMITER ;