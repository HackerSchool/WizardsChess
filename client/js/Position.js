class Position {
    constructor(x,y) {
        if( x==undefined || y ==undefined) {
            this.x = 0;
            this.y = 0;
        }
        else {
            this.x = x;
            this.y = y;
        }
    }
}