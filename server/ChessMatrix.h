#ifndef _CHESS_MATRIX_
#define _CHESS_MATRIX_

#include <utility>
#include <list>
#include <array>
#include "Pieces/Piece.h"
#include "Position.h"
#include <iostream>
class Piece;
class ChessMatrix
{
private:
    const int MAX_X = 8;
    const int MAX_Y = 8;
    //FIXME: Hugo, por que usas MAX_X e MAX_Y, e depois 8 har-coded?
    //FIXME: É um workaround
    std::array< std::array<Piece*,8>, 8> _pieces;
    void oneSideBoardCreation(bool white, bool forward);
public:
    ChessMatrix();

    Piece* get(Position pos)
    {
        //Throw out_of_range    
        return _pieces.at(pos.x).at(pos.y);
    }

    void set(Position pos, Piece* p);

    //Used for a text simulation of the game
    void printMatrix();
};
#endif
