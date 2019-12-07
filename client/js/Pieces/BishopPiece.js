class BishopPiece extends Piece {
    constructor(id,white,pos,g,forward) {
        super(id,white,pos,g,forward);
        this.visual = new BishopPieceVisual();

    }

    getValidMoves() {
        let valid = [];
        let tempX;
        let tempY;
        let pieceInArea = false;

        for( let x = -1; x <= 1; x+=2 ) {
            for( let y = -1; y <= 1; y+=2) {
                tempX = x;
                tempY = y;

                while(pieceInArea == false) {
                    try {
                        pieceInArea = this.pushIfAvailable(valid, new Position(
                            this.myPos.x + tempX, this.myPos.y + tempY), undefined);
                    
                    } catch( err ) { break; }
                } 
            }
        }

        return valid;
    }
}