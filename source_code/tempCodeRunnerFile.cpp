class Player {
    public:
        Player(int x, int y, int width, int height) {
            pos = {(float)x, (float)y};
            vel = {0, 0};
            body = {x, y, width, height};
            isJumping = false;
            isGrounded = false;
        }