CREATE TABLE characters (
    id SERIAL PRIMARY KEY,
    user_id INTEGER,
    name VARCHAR(30) UNIQUE NOT NULL CHECK (name ~ '^[a-zA-Z0-9_]{6,30}$'),
    is_alive BOOLEAN NOT NULL DEFAULT true,
    created TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (user_id) REFERENCES users(id)
);

CREATE INDEX idx_characters_user_id ON characters(user_id);