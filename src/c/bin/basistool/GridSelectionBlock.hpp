#ifndef GRIDSELECTIONBLOCK_HH
#define GRIDSELECTIONBLOCK_HH

class GridSelectionBlock
{
    public:
        GridSelectionBlock();

        void setTopLeft(int row, int col);
        void setBottomRight(int row, int col);

        int getTopRow();
        int getLeftColumn();
        int getBottomRow();
        int getRightColumn();
        string toString();


    private:
        int         p_topRow;
        int         p_leftCol;
        int         p_bottomRow;
        int         p_rightCol;


};

#endif  //  GRIDSELECTIONBLOCK_HH


