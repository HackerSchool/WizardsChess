#include "Server.h"
#include "exceptions/FirstTurnException.h"
#include "exceptions/NotYourTurnException.h"
#include "exceptions/NoSuchPieceException.h"
#include "exceptions/PieceNotYoursException.h"
#include "exceptions/InvalidSessionException.h"
#include "exceptions/InvalidActionException.h"
#include "exceptions/InvalidMoveException.h"
#include "exceptions/PawnPromotionException.h"
#include "exceptions/LogicErrorException.h"
#include "gamestates/GameState.h"
#include "Move.h"
#include "CheckGameVisitor.h"
#include "PawnPromotionStrategy.h"

Server::Server () : _nextGameId (1), _nextSession(1)
{
    
}


std::string Server::process(const std::string& msg, int session) 
{
    std::map<int, Session*>::iterator it;
    
    if ((it = _sessions.find(session)) != _sessions.end()) 
    {
        Session* session = it->second;
        Message* message = _factory.parse(msg);

        std::string answer = message->accept(this, session);
        
        delete message;
        
        return answer;
    }
    
    else
    {
        throw InvalidSessionException();
    }
}

int Server::createSession () 
{
    Session* session = new Session(_nextSession);
    _sessions.insert(std::make_pair(_nextSession, session));
    
    return _nextSession++;
}

void Server::closeSession (int session)
{
    std::map<int, Session*>::iterator it;
    
    if ((it = _sessions.find(session)) != _sessions.end()) 
    {
        Session* session = it->second;
        delete session;
        
        _sessions.erase (it);
    }
    
    else
    {
        throw InvalidSessionException();
    }
}

Player* Server::searchPlayer (const std::string& user)
{
    std::map<std::string, Player*>::iterator it;
    
    if ((it = _players.find(user)) != _players.end())
        return it->second;
    else
        return nullptr;
}

std::string Server::visitReg (RegMessage* message, Session* session) 
{
    
    if (session->isLogged()) 
        return "REG_A ERR LOGGED";
    
    if (searchPlayer(message->user()) == nullptr)
    {
        Player* player = new Player (message->user(), message->pass());
        _players.insert(std::make_pair(message->user(), player));
        
        return "REG_A OK";
    }
    else
        return "REG_A ERR USER_USED";
}

std::string Server::visitListGames (ListGamesMessage* message, Session* session)
{
    Player* player; 
    
    if (!session->isLogged())
        return "LIST_GAMES_A ERR NOT_LOGGED";
    
    if ((player = searchPlayer(session->userName())) != nullptr)
    {
        std::string result ("LIST_GAMES_A OK ");
        
        result += std::to_string(player->games().size());
        
        for (std::pair <int, Game*> pair : player->games())
        {
            Game* game = pair.second;
            
            std::string turn = game->getTurn () ? " W" : " B";
            
            if (game->playerW() == player)
                result += " " + std::to_string(game->gameId()) + " " + 
                    game->playerB()->user() + " W "  + std::to_string(game->turnCount());
            else
                result += " " + std::to_string(game->gameId()) + " " + 
                    game->playerW()->user() + " B "  + std::to_string(game->turnCount());
            
            result += turn;
        }
        
        return result;
    }
    else
        throw LogicErrorException ("User is logged but it is not found.");
}

std::string Server::visitGameMove (GameMoveMessage* message, Session* session)
{
    Player* player;
    Game* game;
    std::string status;
    CheckGameVisitor visitor;
    
    if (!session->isLogged())
        return "LIST_GAMES_A ERR NOT_LOGGED";
    
    if ((player = searchPlayer(session->userName())) != nullptr)
    {
        game = player->searchGame (message->gameId());

        if (game == nullptr)
            return "GAME_MOVE_A ERR GAME_NOT_FOUND";
        
        try 
        {
            game->move (Position (message->x1(), message->y1()), 
                        Position (message->x2(), message->y2()), player->user());
            
            //return the state because it can change after each move
            status = game->getState()->accept(&visitor);
            
            return "GAME_MOVE_A OK " + status;
        }
        catch (NoSuchPieceException& e)
        {
            return "GAME_MOVE_A ERR NO_SUCH_PIECE";
        }
        catch (NotYourTurnException& e)
        {
            return "GAME_MOVE_A ERR NOT_YOUR_TURN";
        }
        catch (PieceNotYoursException& e)
        {
            return "GAME_MOVE_A ERR PIECE_NOT_YOURS";
        }
        catch (PawnPromotionException& e) 
        {
            return "GAME_MOVE_A OK WAITING_FOR_PROMOTION_STATE";
        }
        catch (InvalidMoveException& e)
        {
            return "GAME_MOVE_A ERR INVALID_MOVE";
        }
        catch (InvalidActionException& e)
        {
            //return the state because the error ocurred because the state is not PlayingState
            status = game->getState()->accept(&visitor);
            return "GAME_MOVE_A ERR INVALID_ACTION " + status;
        }
    }
    else 
        throw LogicErrorException ("User is logged but user is not found.");
}

std::string Server::visitGameStatus (GameStatusMessage* message, Session* session)
{
    Player* player;
    CheckGameVisitor visitor;
    
    if (!session->isLogged())
        return "LIST_GAMES_A ERR NOT_LOGGED";
    
    if ((player = searchPlayer(session->userName())) != nullptr)
    {
        Game* game = player->searchGame (message->gameId());

        if (game == nullptr)
            return "GAME_STATUS_A ERR GAME_NOT_FOUND";
        
        return "GAME_STATUS_A OK " + game->getState()->accept(&visitor);
    }
    else 
        throw LogicErrorException ("User is logged but user is not found.");
}

std::string Server::visitGameDrop (GameDropMessage* message, Session* session)
{
    Player* player;
    
    if (!session->isLogged())
        return "LIST_GAMES_A ERR NOT_LOGGED";
    
    if ((player = searchPlayer(session->userName())) != nullptr)
    {   
        Game* game = player->searchGame(message->gameId());
        
        if (game != nullptr) 
        {
            player->removeGame(game->gameId());
            
            if (game->drop (player->user())) 
            {
                Player* other = (game->playerB()->user() == player->user()) ?
                                    game->playerW() : game->playerB ();
                other->removeGame(game->gameId());
                _games.erase (game->gameId());
                delete game;
            }
            
            return "GAME_DROP_A OK";
        }
        
        else 
            return "GAME_DROP_A ERR GAME_NOT_FOUND";
    }
    else 
        throw LogicErrorException ("User is logged but user is not found.");
}

std::string Server::visitGameTurn (GameTurnMessage* message, Session* session)
{
    Player* player;
    
    if (!session->isLogged())
        return "LIST_GAMES_A ERR NOT_LOGGED";
    
    if ((player = searchPlayer(session->userName())) != nullptr)
    {
        Game* game = player->searchGame(message->gameId());
        if (game != nullptr) 
            return game->getTurn () ? "GAME_TURN_A OK W" :
                                        "GAME_TURN_A OK B";
        else
            return "GAME_TURN_A ERR GAME_NOT_FOUND";
    }
    else 
        throw LogicErrorException ("User is logged but user is not found.");
}

std::string Server::visitGameLastMove (GameLastMoveMessage* message, Session* session)
{
    Player* player;
    
    if (!session->isLogged())
        return "LIST_GAMES_A ERR NOT_LOGGED";
    
    if ((player = searchPlayer(session->userName())) != nullptr)
    {
        Game* game = player->searchGame(message->gameId());
        if (game != nullptr) 
        {
            try
            {
                Move move = game->lastMove();
                return "GAME_LAST_MOVE_A OK " 
                        + std::to_string(move.origin.x) + " "
                        + std::to_string(move.origin.y) + " "
                        + std::to_string(move.destination.x) + " "
                        + std::to_string(move.destination.y);
            }
            catch (FirstTurnException& e)
            {
                return "GAME_LAST_MOVE_A ERR FIRST_TURN";
            }                   
        }
        else
            return "GAME_LAST_MOVE_A ERR GAME_NOT_FOUND";
    }
    else 
        throw LogicErrorException ("User is logged but user is not found.");
}

std::string Server::visitPawnPromotion (PawnPromotionMessage* message, Session* session) 
{
    Player* player;
    
    if (!session->isLogged())
        return "LIST_GAMES_A ERR NOT_LOGGED";
    
    if ((player = searchPlayer(session->userName())) != nullptr)
    {
        Game* game = player->searchGame(message->gameId());
        if (game != nullptr) 
        {
            try
            {
                PawnPromotionStrategy *strategy;
                
                if (message->pieceType() == "QUEEN")
                    strategy = new PromoteToQueen;
                
                else if (message->pieceType() == "KNIGTH")
                    strategy = new PromoteToKnight;
                
                else if (message->pieceType() == "ROOK")
                    strategy = new PromoteToRook;
                
                else if (message->pieceType() == "BISHOP")
                    strategy = new PromoteToBishop;
                
                else
                    return "PAWN_PROMOTION_A ERR PIECE_TYPE";
                
                game->promote (strategy);
                
                delete strategy;
                return "PAWN_PROMOTION_A OK";
            }
            catch (InvalidActionException& e)
            {
                return "PAWN_PROMOTION_A ERR INVALID_ACTION";
            }                   
        }
        else
            return "PAWN_PROMOTION_A ERR GAME_NOT_FOUND";
    }
    else 
        throw LogicErrorException ("User is logged but user is not found.");
}

std::string Server::visitNewGame (NewGameMessage* message, Session* session)
{   
    Player *player1, *player2;
    Game* game;
    
    if (!session->isLogged())
        return "LIST_GAMES_A ERR NOT_LOGGED";
    
    player1 = searchPlayer(session->userName());
    player2 = searchPlayer(message->user());
    
    if (player1 == nullptr)
        return "NEW_GAME_A ERR USER_NOT_FOUND";
    
    if (player2 == nullptr)
        throw LogicErrorException ("User is logged but user is not found.");
    
    if (player1->user() == player2->user())
        return "NEW_GAME_A ERR SAME_USER";
    
    if (message->color() == "W") 
        game = new Game (_nextGameId, player1, player2);
    else if (message->color() == "B") 
        game = new Game (_nextGameId, player2, player1);
    else
        return "NEW_GAME_A ERR COLOR";
    
    _games.insert (std::make_pair (_nextGameId, game));
    
    player1->addGame (game);
    player2->addGame (game);
    
    return "NEW_GAME_A OK " + std::to_string(_nextGameId++);
}

std::string Server::visitLogin (LoginMessage* message, Session* session) 
{
    Player* player;
    
    if (session->isLogged())
        return "LOGIN_A ERR ALREADY_LOGGED";
    
    if ((player = searchPlayer(message->user())) != nullptr)
    {
        if (player->validatePassword(message->pass()))
        {
            session->login (message->user());
            return "LOGIN_A OK";
        }
        
        else
            return "LOGIN_A ERR PASS";
    }
    else 
        return "LOGIN_A ERR USER";
}

//IMPORT_GAME_A OK |ENPASSANT|<YES OR NO> (YES) { <POS_X> <POX_Y> <POS_X> <POS_Y> N [ <POS_X> <POS_Y> ](*N) } 
// |KING| <KING_STRING> N |QUEEN| [<QUEENSTRING>](*N) N |PAWN| [<PAWNSTRING>](*N)
// REPEAT
std::string Server::visitImportGame (ImportGameMessage* message, Session* session)
{
    Game* game;
    Player* player;
    
    if (!session->isLogged())
        return "LIST_GAMES_A ERR NOT_LOGGED";
    
    if ((player = searchPlayer(session->userName())) == nullptr)
        throw LogicErrorException ("User is logged but user is not found.");
    
    if ((game = player->searchGame(message->gameId())) != nullptr)
    {
        std::string answer = "IMPORT_GAME_A OK ";

        //game info
        if( ( game->playerW()->user().compare(player->user()) ) == 0 ) {
            answer += "1 " + game->playerB()->user() + " ";
        } else {
            answer += "0 " + game->playerW()->user() + " ";
        }

        answer += std::to_string( game->gameId() ) + " ";
        answer += std::to_string( game->getTurn() ) + " ";

        //enpassant
        if(game->getEnPassantPiece() != nullptr) {
            answer += "YES ";
            answer += std::to_string( game->getEnPassantPiece()->getPos().x ) + " " +
                std::to_string( game->getEnPassantPiece()->getPos().y ) + " " +
                std::to_string( game->getEnPassantDest()->x ) + " " +
                std::to_string( game->getEnPassantDest()->y ) + " " +
                std::to_string( game->getEnPassantLiveTime() ) + " ";

            answer += std::to_string( game->getEnPassantOrigin().size() ) + " ";
            for(Position& pos : game->getEnPassantOrigin() ) {
                answer += std::to_string( pos.x ) + " " + std::to_string( pos.y ) + " ";
            }
        } 
        else {
            answer += "NO ";
        }

        //pieces
        answer += game->getKing(true).stringify() + " ";
        answer += game->getKing(false).stringify() + " ";

        std::list<QueenPiece>& queenW = game->getQueen(true);
        std::list<QueenPiece>& queenB = game->getQueen(false);
        
        std::list<PawnPiece>& pawnW = game->getPawn(true);
        std::list<PawnPiece>& pawnB = game->getPawn(false);

        std::list<RookPiece>& rookW = game->getRook(true);
        std::list<RookPiece>& rookB = game->getRook(false);

        std::list<KnightPiece>& knightW = game->getKnight(true);
        std::list<KnightPiece>& knightB = game->getKnight(false);

        std::list<BishopPiece>& bishopW = game->getBishop(true); 
        std::list<BishopPiece>& bishopB = game->getBishop(false);

        answer += std::to_string( queenW.size() ) + " ";
        for(QueenPiece& p : queenW)
            answer += p.stringify() + " ";

        answer += std::to_string( queenB.size() )+ " ";
        for(QueenPiece& p : queenB)
            answer += p.stringify() + " ";

        //
        answer += std::to_string( pawnW.size() )+ " ";
        for(PawnPiece& p : pawnW)
            answer += p.stringify() + " ";

        answer += std::to_string( pawnB.size() ) + " ";
        for(PawnPiece& p : pawnB)
            answer += p.stringify() + " ";

        //
        answer += std::to_string( rookW.size() ) + " ";
        for(RookPiece& p : rookW)
            answer += p.stringify() + " ";

        answer += std::to_string( rookB.size() )+ " ";
        for(RookPiece& p : rookB)
            answer += p.stringify() + " ";

        //  
        answer += std::to_string( knightW.size() ) + " ";
        for(KnightPiece& p : knightW)
            answer += p.stringify() + " ";

        answer += std::to_string( knightB.size() )+ " ";
        for(KnightPiece& p : knightB)
            answer += p.stringify() + " ";
        //
        answer += std::to_string( bishopW.size() ) + " ";
        for(BishopPiece& p : bishopW)
            answer += p.stringify() + " ";

        answer += std::to_string(  bishopB.size() ) + " ";
        for(BishopPiece& p : bishopB)
            answer += p.stringify() + " ";
        //    

        return answer;
    }
    
    else 
    {
        return "IMPORT_GAME_A ERR GAME_NOT_FOUND";
    }
}

Server::~Server () 
{
    std::map<std::string, Player*>::iterator it1;
    std::map<int, Game*>::iterator it2;
    std::map<int, Session*>::iterator it3; 
    
    for (it1 = _players.begin(); it1 != _players.end(); it1++) 
        delete it1->second;
    
    for (it2 = _games.begin(); it2 != _games.end(); it2++) 
        delete it2->second;
    
    for (it3 = _sessions.begin(); it3 != _sessions.end(); it3++)  
    {
        closeSession(it3->second->sessionId());
        delete it3->second;
    }
}
