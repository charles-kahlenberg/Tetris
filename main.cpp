//Charles Kahlenberg
//created: 10am 5/9/26
//last edit: 10am 5/9/26
//made w/ ai assistance
// Controls (real-time):
//  a : move left
//  d : move right
//  s : soft drop (move down)
//  w : rotate clockwise
//  q : quit
//
// Game ticks automatically; input is processed when available (non-blocking).

#include <bits/stdc++.h>
#include <chrono>
#include <thread>

#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>
#else
    #include <termios.h>
    #include <unistd.h>
#endif

using namespace std;

// --------------------------- Configuration ---------------------------
constexpr int HEIGHT = 30;   // number of rows (0 = top, HEIGHT-1 = bottom)
constexpr int WIDTH  = 10;   // number of columns
constexpr bool APPLY_COLUMN_GRAVITY = true; // if true, floating blocks fall after clears

// Characters used for empty and border rendering
constexpr char EMPTY_CELL = ' ';  // Symbol for empty grid cells
constexpr char BORDER_CELL = '|';  // Symbol for grid borders

// ------------------------- Forward declarations --------------------

struct Cell;           // simple x,y coordinate
struct PieceDef;       // tetromino definition (rotations + symbol)
struct Piece;          // active piece instance (position + rotation)
class Board;           // game board class (40x10 grid)

// Free function forward declarations
Piece spawnRandomPiece();  // Function to generate a new random tetromino
bool tryMove(Piece &p, const Board &b, int dx, int dy);  // Attempts to move a piece by given offsets
bool tryRotate(Piece &p, const Board &b);  // Attempts to rotate a piece clockwise

// --------------------------- Basic types -----------------------------
// Struct representing a single cell position on the board
struct Cell { int x, y; }; // x: column [0..WIDTH-1], y: row [0..HEIGHT-1]

// Piece definition: a set of rotation states, each state is vector<Cell> of relative coords
struct PieceDef {
    char symbol; // display char for this tetromino
    vector<vector<Cell>> rotations; // each rotation is 4 relative cells
};

// Active piece instance
struct Piece {
    const PieceDef* def = nullptr;  // Pointer to the piece's definition
    int rot = 0;    // rotation index
    int x = 0, y = 0;   // position of origin (applied to relative coords)
    vector<Cell> cells() const {  // Method to get absolute cell positions of the piece
        vector<Cell> out;
        for (auto &c : def->rotations[rot]) out.push_back({x + c.x, y + c.y});
        return out;
    }
};

// --------------------------- Tetromino definitions --------------------
// Coordinates are relative to an origin. You can change origin or rotation rules.
// The set below uses common tetromino shapes (I, O, T, L, J, S, Z).
static const vector<PieceDef> TETROMINO_DEFS = {  // Collection of all tetromino types
    // I
    { 'I', {
        { { {0,1}, {1,1}, {2,1}, {3,1} } },
        { { {2,0}, {2,1}, {2,2}, {2,3} } }
    }},
    // O
    { 'O', {
        { { {1,0}, {2,0}, {1,1}, {2,1} } }
    }},
    // T
    { 'T', {
        { { {1,0}, {0,1}, {1,1}, {2,1} } },
        { { {1,0}, {1,1}, {2,1}, {1,2} } },
        { { {0,1}, {1,1}, {2,1}, {1,2} } },
        { { {1,0}, {0,1}, {1,1}, {1,2} } }
    }},
    // L
    { 'L', {
        { { {2,0}, {0,1}, {1,1}, {2,1} } },
        { { {1,0}, {1,1}, {1,2}, {2,2} } },
        { { {0,1}, {1,1}, {2,1}, {0,2} } },
        { { {0,0}, {1,0}, {1,1}, {1,2} } }
    }},
    // J
    { 'J', {
        { { {0,0}, {0,1}, {1,1}, {2,1} } },
        { { {1,0}, {2,0}, {1,1}, {1,2} } },
        { { {0,1}, {1,1}, {2,1}, {2,2} } },
        { { {1,0}, {1,1}, {0,2}, {1,2} } }
    }},
    // S
    { 'S', {
        { { {1,0}, {2,0}, {0,1}, {1,1} } },
        { { {1,0}, {1,1}, {2,1}, {2,2} } }
    }},
    // Z
    { 'Z', {
        { { {0,0}, {1,0}, {1,1}, {2,1} } },
        { { {2,0}, {1,1}, {2,1}, {1,2} } }
    }}
};

// --------------------------- Board class -----------------------------
// Class representing the game board, handling grid state and operations
class Board {
public:
    Board();  // Constructor to initialize the board

    void clearBoard();  // Resets the board to empty state

    // Render the board and optionally the active piece
    void render(const Piece* active = nullptr) const;

    // Check if a set of absolute cells fit on the board and are empty
    bool fits(const vector<Cell>& cells) const;

    // Place piece cells into the board (lock)
    void lockPiece(const Piece& p);

    // Clear full rows, return number of cleared rows
    int clearFullRows();

    // Remove row y and shift everything above down by one
    void removeRow(int row);

    // Optional: make blocks fall straight down in each column (break shapes)
    void applyColumnGravity();

    // Check if any cell in top rows is occupied (game over)
    bool isGameOver() const;

private:
    char board[HEIGHT][WIDTH];
};

// --------------------------- Board implementation --------------------
Board::Board() {
    clearBoard();
}

void Board::clearBoard() {
    for (int y = 0; y < HEIGHT; ++y)
        for (int x = 0; x < WIDTH; ++x)
            board[y][x] = EMPTY_CELL;
}

void Board::render(const Piece* active) const {
    // top border
    cout << '+' << string(WIDTH, '-') << "+\n";
    for (int y = 0; y < HEIGHT; ++y) {
        cout << BORDER_CELL;
        for (int x = 0; x < WIDTH; ++x) {
            char c = board[y][x];
            // overlay active piece if provided
            if (active) {
                for (auto &pc : active->cells()) {
                    if (pc.x == x && pc.y == y) { c = active->def->symbol; break; }
                }
            }
            cout << c;
        }
        cout << BORDER_CELL << '\n';
    }
    // bottom border
    cout << '+' << string(WIDTH, '-') << "+\n";
}

bool Board::fits(const vector<Cell>& cells) const {
    for (auto &c : cells) {
        if (c.x < 0 || c.x >= WIDTH || c.y < 0 || c.y >= HEIGHT) return false;
        if (board[c.y][c.x] != EMPTY_CELL) return false;
    }
    return true;
}

void Board::lockPiece(const Piece& p) {
    for (auto &c : p.cells()) {
        if (c.y >= 0 && c.y < HEIGHT && c.x >= 0 && c.x < WIDTH)
            board[c.y][c.x] = p.def->symbol;
    }
}

int Board::clearFullRows() {
    int cleared = 0;
    for (int y = HEIGHT - 1; y >= 0; --y) {
        bool full = true;
        for (int x = 0; x < WIDTH; ++x) {
            if (board[y][x] == EMPTY_CELL) { full = false; break; }
        }
        if (full) {
            removeRow(y);
            ++cleared;
            ++y; // re-check same y because rows shifted down
        }
    }
    if (cleared > 0 && APPLY_COLUMN_GRAVITY) applyColumnGravity();
    return cleared;
}

void Board::removeRow(int row) {
    for (int y = row; y > 0; --y) {
        for (int x = 0; x < WIDTH; ++x) board[y][x] = board[y-1][x];
    }
    // clear top row
    for (int x = 0; x < WIDTH; ++x) board[0][x] = EMPTY_CELL;
}

void Board::applyColumnGravity() {
    for (int x = 0; x < WIDTH; ++x) {
        int writeY = HEIGHT - 1;
        for (int y = HEIGHT - 1; y >= 0; --y) {
            if (board[y][x] != EMPTY_CELL) {
                board[writeY][x] = board[y][x];
                if (writeY != y) board[y][x] = EMPTY_CELL;
                --writeY;
            }
        }
        // fill remaining cells above with empty
        for (int y = writeY; y >= 0; --y) board[y][x] = EMPTY_CELL;
    }
}

bool Board::isGameOver() const {
    for (int x = 0; x < WIDTH; ++x) if (board[0][x] != EMPTY_CELL) return true;
    return false;
}

// --------------------------- Utility functions -----------------------
static std::mt19937 rng((unsigned)chrono::system_clock::now().time_since_epoch().count());

// Create a new random piece positioned near the top center
Piece spawnRandomPiece() {
    const PieceDef* def = &TETROMINO_DEFS[rng() % TETROMINO_DEFS.size()];
    Piece p;
    p.def = def;
    p.rot = 0;
    // spawn near top center; adjust origin so piece fits
    p.x = (WIDTH / 2) - 2;
    p.y = 0;
    return p;
}

// Attempt to move piece by dx,dy; return true if moved
bool tryMove(Piece &p, const Board &b, int dx, int dy) {
    Piece tmp = p;
    tmp.x += dx; tmp.y += dy;
    if (b.fits(tmp.cells())) { p = tmp; return true; }
    return false;
}

// Attempt to rotate piece clockwise; return true if rotated
bool tryRotate(Piece &p, const Board &b) {
    Piece tmp = p;
    tmp.rot = (tmp.rot + 1) % tmp.def->rotations.size();
    // simple wall-kick: try no kick, left, right, up
    if (b.fits(tmp.cells())) { p = tmp; return true; }
    tmp.x -= 1; if (b.fits(tmp.cells())) { p = tmp; return true; }
    tmp.x += 2; if (b.fits(tmp.cells())) { p = tmp; return true; }
    tmp.x -= 1; tmp.y -= 1; if (b.fits(tmp.cells())) { p = tmp; return true; }
    return false;
}

// Non-blocking input function: returns a character if available, '\0' otherwise
char getNonBlockingInput() {
#ifdef _WIN32
    if (_kbhit()) {
        return _getch();
    }
    return '\0';
#else
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    
    struct timeval tv = {0, 0};  // Non-blocking timeout
    int result = select(STDIN_FILENO + 1, &readfds, nullptr, nullptr, &tv);
    
    char c = '\0';
    if (result > 0) {
        read(STDIN_FILENO, &c, 1);
    }
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return c;
#endif
}

// --------------------------- Main loop -------------------------------
int main() {
    Board board;  // Create game board instance
    Piece active = spawnRandomPiece();  // Spawn initial active piece

    // If spawn collides immediately, game over
    if (!board.fits(active.cells())) {
        cout << "Game over on spawn. Board too full.\n";
        return 0;
    }

    int score = 0;  // Player's score
    int tick = 0;   // Game tick counter
    auto lastTickTime = chrono::high_resolution_clock::now();  // Track time for automatic ticks
    const int TICK_INTERVAL_MS = 500;  // Milliseconds between automatic ticks (adjust for difficulty)

    cout << "Tetris - Auto-ticking game. Controls: a,d,s,w,q\n";
    cout.flush();

    while (true) {  // Main game loop
        auto now = chrono::high_resolution_clock::now();
        auto elapsed = chrono::duration_cast<chrono::milliseconds>(now - lastTickTime).count();

        // Check for user input (non-blocking)
        char cmd = getNonBlockingInput();
        if (cmd == 'q') break;

        // Process player input even if tick hasn't occurred yet
        bool moved = false;
        if (cmd == 'a') moved = tryMove(active, board, -1, 0);
        else if (cmd == 'd') moved = tryMove(active, board, +1, 0);
        else if (cmd == 's') moved = tryMove(active, board, 0, +1);
        else if (cmd == 'w') { tryRotate(active, board); }

        // Execute tick if enough time has passed
        if (elapsed >= TICK_INTERVAL_MS) {
            lastTickTime = now;

            // Clear screen and render
            system("cls"); // on Windows; use "clear" on Linux/Mac
            cout << "Score: " << score << "  Tick: " << tick << "\n";
            board.render(&active);
            cout << "Controls: a,d,s,w,q  |  Tick interval: " << TICK_INTERVAL_MS << "ms\n";
            cout.flush();

            // Gravity: automatic downward move each tick
            bool fell = tryMove(active, board, 0, +1);

            // If piece can't fall, lock it
            if (!fell) {
                // If piece cannot move down, lock it
                board.lockPiece(active);
                int cleared = board.clearFullRows();
                score += cleared * 100;
                // spawn new piece
                active = spawnRandomPiece();
                if (!board.fits(active.cells())) {
                    system("cls");
                    board.render(nullptr);
                    cout << "Game Over! Final score: " << score << "\n";
                    break;
                }
            }

            ++tick;
        } else {
            // Small sleep to avoid CPU spinning
            this_thread::sleep_for(chrono::milliseconds(10));
        }
    }

    cout << "Exiting. Final score: " << score << "\n";
    return 0;
}