#ifndef MOVE_H
#define MOVE_H

#include "Types.h" // Include our new types header for GamePoint, PlayerColor, PieceTypeIndex

// The Move struct encapsulates all information about a chess move.
// This makes move generation, application, and undo cleaner and more explicit.
struct Move {
    // Starting square of the piece that moves.
    // Example: For e2e4, from_square would be {x:4, y:1} (assuming A1=0, H8=7 map for y)
    GamePoint from_square; 
    
    // Destination square of the piece that moves.
    // Example: For e2e4, to_square would be {x:4, y:3}
    GamePoint to_square;   

    // Type of the piece that is moving (e.g., PieceTypeIndex::PAWN, PieceTypeIndex::KNIGHT).
    // This *does not* include color. Color is derived from the active player.
    PieceTypeIndex piece_moved_type_idx; 
    
    // Type of the piece that is captured, if any.
    // Use PieceTypeIndex::NONE if no piece is captured.
    PieceTypeIndex piece_captured_type_idx; 

    // --- Special Move Flags ---
    // These boolean flags indicate specific characteristics of the move.

    // True if this move is a pawn promotion.
    bool is_promotion;        
    // If is_promotion is true, this indicates the type of piece the pawn promotes to.
    // (e.g., PieceTypeIndex::QUEEN, PieceTypeIndex::KNIGHT).
    PieceTypeIndex promotion_piece_type_idx;
    
    // True if this move is a kingside castle.
    bool is_kingside_castle;  
    // True if this move is a queenside castle.
    bool is_queenside_castle; 
    
    // True if this move is an en passant capture.
    bool is_en_passant;       
    
    // True if this move is a pawn moving two squares from its starting rank.
    bool is_double_pawn_push; 

    // Constructor for creating a Move object.
    // All parameters are initialized for clear move construction.
    Move(GamePoint from, GamePoint to, 
         PieceTypeIndex moved_type, PieceTypeIndex captured_type = PieceTypeIndex::NONE,
         bool promotion = false, PieceTypeIndex promo_type = PieceTypeIndex::NONE,
         bool k_castle = false, bool q_castle = false,
         bool en_pass = false, bool double_push = false)
        : from_square(from), to_square(to), 
          piece_moved_type_idx(moved_type), piece_captured_type_idx(captured_type),
          is_promotion(promotion), promotion_piece_type_idx(promo_type),
          is_kingside_castle(k_castle), is_queenside_castle(q_castle),
          is_en_passant(en_pass), is_double_pawn_push(double_push) {}

    // Example Usage:

    // 1. Normal Pawn Move (e2e4)
    // GamePoint e2 = {4, 1}; // File 'e' (4), Rank '2' (1)
    // GamePoint e4 = {4, 3}; // File 'e' (4), Rank '4' (3)
    // Move pawn_move_e2e4(e2, e4, PieceTypeIndex::PAWN, PieceTypeIndex::NONE, false, PieceTypeIndex::NONE, false, false, false, true);
    // (Note: The last 'true' indicates it's a double pawn push)

    // 2. Knight Move (g1f3)
    // GamePoint g1 = {6, 0}; // File 'g' (6), Rank '1' (0)
    // GamePoint f3 = {5, 2}; // File 'f' (5), Rank '3' (2)
    // Move knight_move_g1f3(g1, f3, PieceTypeIndex::KNIGHT); // Default values for captured_type and flags are sufficient

    // 3. Capture (Nxf6) - assuming White Knight captures Black Pawn on f6
    // GamePoint d4 = {3, 3}; // d4 (Example Knight from d4)
    // GamePoint f6 = {5, 5}; // f6 (Captured piece on f6)
    // Move capture_Nxf6(d4, f6, PieceTypeIndex::KNIGHT, PieceTypeIndex::PAWN); // Only specify moved and captured types

    // 4. Kingside Castling (White: O-O, King moves E1 to G1)
    // GamePoint e1 = {4, 0}; // E1
    // GamePoint g1 = {6, 0}; // G1
    // Move white_kingside_castle(e1, g1, PieceTypeIndex::KING, PieceTypeIndex::NONE, false, PieceTypeIndex::NONE, true);

    // 5. Pawn Promotion (e7e8Q) - assuming Black Pawn on e7 promotes to Queen on e8
    // GamePoint e7 = {4, 6}; // E7
    // GamePoint e8 = {4, 7}; // E8
    // Move black_pawn_promo_Q(e7, e8, PieceTypeIndex::PAWN, PieceTypeIndex::NONE, true, PieceTypeIndex::QUEEN);

    // 6. En Passant Capture (exd6 e.p.) - assuming White pawn on e5 captures Black pawn on d6
    // GamePoint e5 = {4, 4}; // E5 (White Pawn)
    // GamePoint d6 = {3, 5}; // D6 (Target square)
    // Move en_passant_exd6(e5, d6, PieceTypeIndex::PAWN, PieceTypeIndex::PAWN, false, PieceTypeIndex::NONE, false, false, true);
};

#endif // MOVE_H
