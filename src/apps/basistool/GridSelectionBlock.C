#include <string>
    using std::string;

#include "util/StringConverter.H"

#include "GridSelectionBlock.H"


GridSelectionBlock::GridSelectionBlock()
{
    p_topRow = 0;
    p_leftCol = 0;
    p_bottomRow = 0;
    p_rightCol = 0;
}


void GridSelectionBlock::setTopLeft(int row, int col)
{
    p_topRow = row;
    p_leftCol = col;

    if (p_bottomRow < p_topRow)
        p_bottomRow = p_topRow;

    if (p_rightCol < p_leftCol)
        p_rightCol = p_leftCol;
}


void GridSelectionBlock::setBottomRight(int row, int col)
{
    p_bottomRow = row;
    p_rightCol = col;

    if (p_topRow > p_bottomRow)
        p_topRow = p_bottomRow;

    if (p_leftCol > p_rightCol)
        p_leftCol = p_rightCol;
}


string GridSelectionBlock::toString()
{
    string s = "(" + StringConverter::toString(p_topRow)
             + ", " + StringConverter::toString(p_leftCol)
             + ")->(" + StringConverter::toString(p_bottomRow)
             + ", " + StringConverter::toString(p_rightCol) + ")";

    return s;
}


int GridSelectionBlock::getTopRow()
{
    return p_topRow;
}


int GridSelectionBlock::getLeftColumn()
{
    return p_leftCol;
}


int GridSelectionBlock::getBottomRow()
{
    return p_bottomRow;
}


int GridSelectionBlock::getRightColumn()
{
    return p_rightCol;
}
