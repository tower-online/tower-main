DELIMITER //
CREATE PROCEDURE insert_dummies(n INT)
BEGIN
    DECLARE i INT DEFAULT 1;

    WHILE i <= n DO
        INSERT INTO users (username, platform)
        VALUES (
            CONCAT('dummy_', LPAD(i, 5, '0')),
            'TEST'
        );

        SET i = i + 1;
    END WHILE;
END //
DELIMITER ;

CALL insert_dummies(1000);
DROP PROCEDURE IF EXISTS insert_dummies;


DELIMITER //
CREATE PROCEDURE insert_dummy_characters(n INT)
BEGIN
    DECLARE i INT DEFAULT 1;
    DECLARE dummy_name VARCHAR(30);

    WHILE i <= n DO
        SET dummy_name = CONCAT('dummy_', LPAD(i, 5, '0'));
        SELECT id INTO @user_id
        FROM users
        WHERE username = dummy_name;

        INSERT INTO characters (user_id, name, race)
        VALUES (
            @user_id,
            dummy_name,
            'HUMAN'
        );

        SET i = i + 1;
    END WHILE;
END;
DELIMITER ;

CALL insert_dummy_characters(1000);
DROP PROCEDURE IF EXISTS insert_dummy_characters;