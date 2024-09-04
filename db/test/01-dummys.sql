CREATE PROCEDURE insert_dummys()
BEGIN
    DECLARE i INT DEFAULT 1;

    WHILE i <= 1000 DO
    INSERT INTO users (username, platform)
    VALUES (
        CONCAT('dummy_', LPAD(i, 5, '0')),
        'TEST'
    );

    SET i = i + 1;
    END WHILE;
END;

CALL insert_dummy_users();
DROP PROCEDURE IF EXISTS insert_dummys;