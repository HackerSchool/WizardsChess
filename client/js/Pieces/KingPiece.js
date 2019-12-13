class KingPiece extends Piece {
    constructor(id,white,pos,g,forward) {
        super(id,white,pos,g,forward);
        this.visual = new KingPieceVisual(this);

        this.hasMoved = false;
    }

    getValidMoves() {
        let valid = [];

        for( let x = -1; x <= 1; x++ ) {
            for( let y = -1; y <= 1; y++) {
                if(x == 0 && y == 0) continue;
                try {
                    pieceInArea = this.pushIfAvailable(valid, new Position(
                        this.pos.x + tempX, this.pos.y + tempY), undefined);
                
                } catch( err ) { continue; }
            }
        }

        return valid;
    }

    getValidCastling() {
        if(this.hasMoved) return [];

        let valid = [];
        rooks = this.game.getRook(this.white);

        for(let rook in rooks) {
            if(rook.hasMoved()) continue;

            let rPos = rook.pos;
            let dx = this.pos.x - rPos.x < 0 ? 1 : -1;
            let x = this.pos.x + dx;
            let y = this.pos.y;

            while(x!=rPos.x) {
                if(this.game.getCell( new Position(x,y) ) != null ) {
                    break;
                }

                x+=dx;
            }

            if(x==rPos.x) {
                valid.push( new Position(this.pos.x + dx*2, y));
            }
        }

        return valid;
    }

    validateCastling(dest) {
        for(let pos in this.getValidCastling() ) {
            if(pos.x == dest.x && pos.y == dest.y) {
                return true;
            }
        }
        return false;
    }

    getCastlingRook(dest) {
        if(this.validateCastling(dest)==false) return null;
        
        //FIXME (Same FIXME In c++)
        const MAX_X = 7;
        const MIN_X = 0;
        if(dest.x == this.pos + 2) {
            return this.game.getCell( new Position(MAX_X ,this.pos.y));
        }
        else {
            return this.game.getCell( new Position(MIN_X, this,pos.y));
        }
    }

    setPos(pos) {
        super.setPos(pos);
        this.hasMoved = true;
    }

    hasMoved() { return this.hasMoved; }

    update(deltaTime) {
        this.visual.update(deltaTime);
    }
}