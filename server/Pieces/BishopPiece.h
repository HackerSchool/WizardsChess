#include "Piece.h"
#include <list>
class BishopPiece : Piece
{
public:
    BishopPiece(int id, bool white, Position pos, ChessMatrix* m,
        bool forward): Piece(id,white,pos,m,forward) {}
        
    virtual std::list<Position> getValidMoves();

    virtual void debugPrint() { std::cout << "B"; }
};