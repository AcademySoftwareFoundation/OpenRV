//
//  Copyright (c) 2010 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPBaseNodes/LayoutGroupIPNode.h>
#include <IPBaseNodes/PaintIPNode.h>
#include <IPBaseNodes/StackIPNode.h>
#include <IPCore/AdaptorIPNode.h>
#include <IPCore/IPGraph.h>
#include <IPCore/NodeDefinition.h>
#include <IPCore/Transform2DIPNode.h>
#include <TwkContainer/Properties.h>
#include <TwkUtil/File.h>
#include <TwkUtil/sgcHop.h>
#include <cmath>

namespace IPCore
{
    using namespace std;
    using namespace TwkContainer;
    using namespace TwkMath;

    string LayoutGroupIPNode::m_defaultMode = "packed";
    bool LayoutGroupIPNode::m_defaultAutoRetime = true;

    LayoutGroupIPNode::LayoutGroupIPNode(const std::string& name,
                                         const NodeDefinition* def,
                                         IPGraph* graph, GroupIPNode* group)
        : GroupIPNode(name, def, graph, group)
        , m_layoutRequested(false)
    {
        declareProperty<StringProperty>("ui.name", name);

        m_mode = declareProperty<StringProperty>("layout.mode", m_defaultMode);
        m_retimeToOutput = declareProperty<IntProperty>(
            "timing.retimeInputs", m_defaultAutoRetime ? 1 : 0);
        m_spacing = declareProperty<FloatProperty>("layout.spacing", 1.0f);
        m_gridRows = declareProperty<IntProperty>("layout.gridRows", 0);
        m_gridColumns = declareProperty<IntProperty>("layout.gridColumns", 0);
        m_stackNode = newMemberNodeOfType<StackIPNode>(stackType(), "stack");
        m_paintNode = newMemberNodeOfType<PaintIPNode>(paintType(), "paint");

        m_paintNode->setInputs1(m_stackNode);
        m_stackNode->setFitInputsToOutputAspect(false);
        setRoot(m_paintNode);
    }

    LayoutGroupIPNode::~LayoutGroupIPNode()
    {
        //
        //  The GroupIPNode will delete the root
        //
    }

    string LayoutGroupIPNode::retimeType()
    {
        return definition()->stringValue("defaults.retimeType", "Retime");
    }

    string LayoutGroupIPNode::transformType()
    {
        return definition()->stringValue("defaults.transformType",
                                         "Transform2D");
    }

    string LayoutGroupIPNode::stackType()
    {
        return definition()->stringValue("defaults.stackType", "Stack");
    }

    string LayoutGroupIPNode::paintType()
    {
        return definition()->stringValue("defaults.paintType", "Paint");
    }

    IPNode* LayoutGroupIPNode::newSubGraphForInput(size_t index,
                                                   const IPNodes& newInputs)
    {
        layoutIfRequested();

        float fps = m_stackNode->imageRangeInfo().fps;
        if (fps == 0.0 && !newInputs.empty())
        {
            auto const imageRangeInfo = newInputs[index]->imageRangeInfo();
            if (!imageRangeInfo.isUndiscovered)
                fps = imageRangeInfo.fps;
        }

        bool retimeInputs = m_retimeToOutput->front() ? true : false;

        IPNode* in = newInputs[index];
        AdaptorIPNode* anode = newAdaptorForInput(in);
        Transform2DIPNode* tnode =
            newMemberNodeOfTypeForInput<Transform2DIPNode>(transformType(), in,
                                                           "t");

        tnode->setAdaptiveResampling(true);
        tnode->setInputs1(anode);
        IPNode* node = tnode;

        if (fps != 0.0 && retimeInputs)
        {
            IPNode* retimer = newMemberNodeForInput(retimeType(), in, "rt");
            retimer->setInputs1(node);
            retimer->setProperty<FloatProperty>("output.fps", fps);
            node = retimer;
        }

        return node;
    }

    IPNode* LayoutGroupIPNode::modifySubGraphForInput(size_t index,
                                                      const IPNodes& newInputs,
                                                      IPNode* subgraph)
    {
        layoutIfRequested();

        bool retimeInputs = m_retimeToOutput->front() ? true : false;

        //
        //  Delete the existing retime node if there is one and make a new
        //  one if needed.
        //

        if (retimeInputs && subgraph->protocol() != retimeType())
        {
            auto const inputRangeInfo = newInputs[index]->imageRangeInfo();
            if (!inputRangeInfo.isUndiscovered)
            {
                IPNode* innode = newInputs[index];
                IPNode* retimer =
                    newMemberNodeForInput(retimeType(), innode, "rt");
                retimer->setInputs1(subgraph);
                retimer->setProperty<FloatProperty>("output.fps",
                                                    inputRangeInfo.fps);
                subgraph = retimer;
            }
        }

        if (subgraph->protocol() == retimeType())
        {
            float retimeFPS = 0.0f;
            if (retimeInputs)
                retimeFPS = m_stackNode->outputFPSProperty()->front();
            else
                retimeFPS = newInputs[index]->imageRangeInfo().fps;

            subgraph->setProperty<FloatProperty>("output.fps", retimeFPS);
            m_stackNode->invalidate();
        }

        return subgraph;
    }

    //----------------------------------------------------------------------
    //
    //  Anonyous namespace
    //
    //  HOW THE LAYOUT WORKS
    //  --------------------
    //
    //  The LayoutGroupIPNode computes the layout of LayoutItem
    //  objects. So if you want to layout something (IPImages or
    //  Transform2DIPNode values) you need to convert to LayoutItem first
    //  then run the various algorithms.
    //
    //  For row, column, packed, and grid modes, the group evaluate() is
    //  intercepted and the layout occurs at runtime on IPImages that make
    //  up the layout (these are children of the underlying stack
    //  node). For static modes the evaluation is a pass through and all
    //  of the layout information is contained in each inputs sub-graph
    //  Transform2DIPNode.
    //
    //  In order to maintain consistancy, whenever the layout mode prop
    //  changes, the layout if performed on the Transform2DIPNodes
    //  first. In the non-static cases this is then overriden by the
    //  evaluate(). When the user switches back into non-static mode the
    //  Transform2DIPNodes will have a value from the last interactive
    //  layout mode.
    //

    namespace
    {

        class LayoutItem
        {
        public:
            LayoutItem(float w = 0.0f, float h = 0.0f, float s = 1.0f,
                       float xt = 0.0f, float yt = 0.0f, float r = 0.0f)
                : width(w)
                , height(h)
                , scale(s)
                , xtrans(xt)
                , ytrans(yt)
                , rotation(r)
            {
            }

            void set(float x, float y, float s, float r = 0.0f)
            {
                xtrans = x;
                ytrans = y;
                scale = s;
                rotation = r;
            }

            float width;
            float height;
            float scale;
            float xtrans;
            float ytrans;
            float rotation;
        };

        typedef vector<LayoutItem> LayoutItems;
        typedef vector<LayoutItem*> LayoutItemPointers;

        void geometryAtNode(int frame, IPNode* node, float& width,
                            float& height)
        {
            IPNode::ImageStructureInfo i =
                node->imageStructureInfo(node->graph()->contextForFrame(frame));
            width = (i.height > 0) ? float(i.width) / float(i.height) : 0;
            height = 1.0;
        }

        void convertToLayoutItems(int frame,
                                  const IPNode::MetaEvalInfoVector& infos,
                                  LayoutItems& items,
                                  LayoutItemPointers& itemPointers)
        {
            items.resize(infos.size());
            itemPointers.resize(infos.size());

            for (size_t i = 0; i < infos.size(); i++)
            {
                geometryAtNode(frame, infos[i].node, items[i].width,
                               items[i].height);
                itemPointers[i] = &items[i];
            }
        }

        void setTransform(IPNode* node, float x, float y, float s, float r)
        {
            node->setProperty<Vec2fProperty>("transform.translate",
                                             Vec2f(x, y));
            node->setProperty<Vec2fProperty>("transform.scale", Vec2f(s, s));
            node->setProperty<FloatProperty>("transform.rotate", r);
        }

        void setTransforms(const IPNode::MetaEvalInfoVector& infos,
                           const LayoutItems& items)
        {
            for (size_t i = 0; i < infos.size(); i++)
            {
                const LayoutItem& item = items[i];
                setTransform(infos[i].node, item.xtrans, item.ytrans,
                             item.scale, item.rotation);
            }
        }

        void identityAllItems(LayoutItemPointers& items)
        {
            for (size_t i = 0; i < items.size(); i++)
            {
                LayoutItem* item = items[i];

                item->scale = 1;
                item->xtrans = 0;
                item->ytrans = 0;
                item->rotation = 0;
            }
        }

        void scaleAllItems(LayoutItemPointers& items, float s)
        {
            for (size_t i = 0; i < items.size(); i++)
            {
                LayoutItem* item = items[i];

                item->scale *= s;
            }
        }

        float maxItemWidth(const LayoutItemPointers& items)
        {
            float maxW = 0;
            for (size_t i = 0; i < items.size(); i++)
            {
                maxW = max(maxW, items[i]->width);
            }

            return maxW;
        }

        void layoutItemsInGrid(LayoutItemPointers& items, float aspect,
                               int cols, int rows)
        {
            if (items.size() == 0)
                return;

            //
            // Make sure the number of rows and columns is positive and no
            // larger than the number of items.
            //

            if (cols > items.size())
                cols = items.size();
            if (rows > items.size())
                rows = items.size();
            if (cols < 0)
                cols = 0;
            if (rows < 0)
                rows = 0;

            float maxW =
                maxItemWidth(items); // Use widest element for column width

            //
            // There are three possible cases for using layoutItemsInGrid
            //
            // case #1; cols & rows are zero
            //  If neither the columns nor the rows are defined then we are
            //  going to automatically figure out the grid layout. The number of
            //  columns and rows we choose is the number that provides the
            //  largest scale. In other words we try all possible col x row
            //  layouts and choose the one where the items are as large as
            //  possible. The automatic layout will be a column fixed layout.
            //
            // case #2; cols is zero but rows is defined
            //  If we have a fixed set of rows then we calculate the number of
            //  columns required to display all of the items.
            //
            // case #3; cols is defined (rows is ignored/calculated)
            //  If we have a fixed set of columns then we calculate the number
            //  of rows required to display all of the items.
            //

            bool fixedRows = false;
            if (cols != 0)
            {
                rows = int(ceil(double(items.size()) / double(cols)));
            }
            else
            {
                if (rows != 0)
                {
                    fixedRows = true;
                    cols = int(ceil(double(items.size()) / double(rows)));
                }
                else
                {
                    float bestScale = 0;
                    for (int colsC = 1; colsC < items.size(); colsC++)
                    {
                        int rowsC =
                            int(ceil(double(items.size()) / double(colsC)));
                        float scaleC = min(aspect / (float(colsC) * maxW),
                                           1.0f / float(rowsC));
                        if (scaleC > bestScale)
                        {
                            bestScale = scaleC;
                            cols = colsC;
                            rows = rowsC;
                        }
                    }
                }
            }

            //
            // The scale is the minimum of fitting the grid by width or by
            // height.
            //

            float scale =
                min(aspect / (float(cols) * maxW), 1.0f / float(rows));

            //
            // If the layout is fixed by row count then we need to make sure to
            // fill rows before columns. If the layout has a fixed number of
            // columns then we need to do the opposite and make sure we hit each
            // column before we worry about the rows.
            //

            size_t n = 0;
            int out = (fixedRows) ? cols : rows;
            int in = (fixedRows) ? rows : cols;
            for (int o = 0; o < out; o++)
            {
                for (int i = 0; i < in; i++)
                {
                    if (n == items.size())
                        return;
                    LayoutItem* item = items[n++];
                    int c = (fixedRows) ? o : i;
                    int r = (fixedRows) ? i : o;
                    float x = (((float(cols) - 1.0) / -2.0) + float(c)) * maxW
                              * scale;
                    float y = (((float(rows) - 1.0) / -2.0) + float(r)) * scale;
                    item->set(x, -1.0 * y, scale);
                }
            }
        }

        float rowWidthOfItems(const LayoutItemPointers& items)
        {
            float xtotal = 0;

            if (items.size() > 1) // the case 1 xtotal is 0
            {
                for (size_t i = 0; i < items.size(); i++)
                {
                    xtotal += items[i]->width;
                }
            }

            return xtotal;
        }

        float layoutRowOfItems(LayoutItemPointers& items, float y, float s,
                               float off)
        {
            float xaccum = 0;

            for (size_t i = 0; i < items.size(); i++)
            {
                LayoutItem* item = items[i];
                float hwidth = item->width / 2.0;

                if (i > 0)
                    xaccum += hwidth;
                float x = (xaccum - off) * s;
                xaccum += hwidth;

                if (items.size() == 1)
                    x = 0;

                item->set(x, y, s);
            }

            return xaccum;
        }

        bool isAbleToLayoutRows(LayoutItemPointers& items, const float aspect,
                                const size_t rowNo, size_t& actualNo)
        {
            size_t rowCount = 1;
            float curRow = 0;
            const float rowCap = aspect * rowNo;
            for (size_t i = 0; i < items.size();)
            {
                if (curRow + items[i]->width > rowCap)
                {
                    rowCount++;
                    curRow = 0;
                }
                else
                {
                    curRow += items[i]->width;
                    i++;
                }
                if (rowCount > rowNo)
                {
                    return false;
                }
            }
            actualNo = rowCount;
            return true;
        }

        void layoutItemsPacked(LayoutItemPointers& items, float aspect)
        {
            //
            // this function figures out how to layout items in a way that
            // satifies the following conditions: i. the layout will maintain
            // the original order of the items ii. there is no empty space (gap)
            // between consecutive items in any row,
            //     and there is no empty space between consecutive rows.
            // iii. all the items have to fit in the 'target' rendering size
            //
            // given the above constraints, this algorithm tries to find a
            // 'optimal' layout that will minimize empty space and balance out
            // empty space among rows
            //

            if (!items.size())
                return;

            //
            // first add all incoming aspects
            //

            float suma = 0;
            for (size_t i = 0; i < items.size(); i++)
            {
                suma += items[i]->width;
            }

            //
            // let m be the number of rows, each row has a aspect ratio of
            // aspect * m, and there are m of them. so the total aspect that
            // this division can fit is m * m * aspect
            // this number has to be
            // n is the minimum number of rows we will need. but n might not
            // work out
            //

            size_t rows = ceil(sqrt(suma / aspect));
            size_t actualRows = 0;
            while (!isAbleToLayoutRows(items, aspect, rows, actualRows))
            {
                rows++;
            }

            //
            // attempt to lay things out greedy way
            //

            const float rowa = rows * aspect;
            LayoutItemPointers tempRow;
            vector<size_t> markers;   // row begin and end markers
            vector<float> rowaspects; // keeps the total aspect of each row
            markers.push_back(0);
            float curRow = 0;
            for (size_t row = 0, i = 0; row < actualRows; row++)
            {
                curRow = 0;
                for (; i < items.size(); ++i)
                {
                    if (curRow + items[i]->width > rowa)
                    {
                        rowaspects.push_back(curRow);
                        break;
                    }
                    curRow += items[i]->width;
                }
                markers.push_back(i);
            }
            rowaspects.push_back(curRow);

            //
            // rearrange to balance out empty space. try to evenly distribute
            // the empty space only allowed to move the last to next row don't
            // need to think about moving first to last row, because if it was
            // possible, it would have been so
            //

            bool madechanges = true;
            while (madechanges)
            {
                madechanges = false;
                for (size_t row = 0; row < actualRows; row++)
                {
                    // consider moving the last element if it helps balance out
                    // empty space
                    if (row != actualRows - 1)
                    {
                        const float w = items[markers[row + 1] - 1]->width;
                        if (w + rowaspects[row + 1] <= rowa)
                        {
                            const float es = max(rowa - rowaspects[row + 1],
                                                 rowa - rowaspects[row]);
                            const float es2 =
                                max(rowa - rowaspects[row + 1] - w,
                                    rowa - rowaspects[row] + w);
                            if (es2 < es)
                            {
                                markers[row + 1]--;
                                rowaspects[row + 1] += w;
                                rowaspects[row] -= w;
                                madechanges = true;
                            }
                        }
                    }
                }
            }

            float factor = 1.0;
            float sy = float(1.0) / float(rows);
            if (actualRows != rows)
            {

                //
                // find out the max row aspect, if it is less than the allowed
                // we can scale up until it is snug
                //

                float lra = 0;
                for (size_t row = 0; row < actualRows; row++)
                {
                    if (rowaspects[row] > lra)
                        lra = rowaspects[row];
                }
                if (lra < rowa)
                {
                    factor = rowa / lra;
                    sy *= factor;
                }
            }

            //
            // this is when we lay out items for real
            //

            for (size_t row = 0; row < actualRows; row++)
            {
                tempRow.clear();
                for (size_t i = markers[row]; i < markers[row + 1]; i++)
                {
                    tempRow.push_back(items[i]);
                }
                layoutRowOfItems(
                    tempRow, 0.5 * sy * actualRows - 0.5 * sy - row * sy, sy,
                    (rowWidthOfItems(tempRow) / 2.0
                     - tempRow.front()->width / 2.0));
            }
        }

        void layoutItemsPacked2(LayoutItemPointers& items, float aspect,
                                int fixedRows = 0)
        {
            float totalWidth = rowWidthOfItems(items);
            const float root = std::sqrt(totalWidth / aspect);
            const size_t rows = fixedRows ? fixedRows : std::ceil(root);
            const float rowWidth = aspect;
            const float s = 1.0f / rows;

            LayoutItemPointers tempRow;

            size_t i = 0;

            for (int row = 0; row < rows; row++)
            {
                float y = float(rows - row - 1) * s - float(rows) * s + s;
                tempRow.clear();

                float tw = 0.0f;

                while (i < items.size())
                {
                    tw += items[i]->width * s;
                    if (tw > rowWidth)
                        break;
                    tempRow.push_back(items[i]);
                    i++;
                }

                const size_t n = tempRow.size();
                if (n > 0)
                {
                    const float hwidth = tempRow.front()->width / 2.0f;

                    layoutRowOfItems(tempRow,
                                     y + rows * s / 2.0 - 1.0 / rows / 2.0f, s,
                                     rowWidth / 2.0 / s - hwidth);

                    if (n > 1)
                    {
                        const float rowActual = rowWidthOfItems(tempRow) * s;
                        const float blankSpace = rowWidth - rowActual;
                        const float spacing = blankSpace / float(n - 1);

                        for (size_t q = 1; q < tempRow.size(); q++)
                        {
                            tempRow[q]->xtrans += spacing * float(q);
                        }
                    }
                }
            }

            if (i < items.size())
            {
                //
                //  If we get here the row approximation failed and we need to
                //  add a row
                //

                layoutItemsPacked2(items, aspect, rows + 1);
            }
        }

    } // namespace

    //----------------------------------------------------------------------

    void LayoutGroupIPNode::layout()
    {
        HOP_PROF_FUNC();
        const string& m = m_mode->front();

        //
        //  NOTE: Images are *centered* -- so their origin is the center of the
        //  image. So we need to offset by the 1/2 the first image's width in
        //  order to make the row flush
        //

        int frame = graph()->cache().displayFrame();
        MetaEvalInfoVector infos;
        MetaEvalClosestByTypeName evaluator(infos, transformType());
        metaEvaluate(contextForFrame(frame), evaluator);

        LayoutItems items;
        LayoutItemPointers itemPointers;
        convertToLayoutItems(frame, infos, items, itemPointers);

        ImageStructureInfo geom =
            imageStructureInfo(graph()->contextForFrame(frame));
        float aspect = float(geom.width) / float(geom.height);
        if (std::isnan(aspect))
            aspect = 16.0 / 9.0;

        bool setTrans = true;
        bool doSpacing = true;
        int cols = m_gridColumns->front();
        int rows = m_gridRows->front();

        if (m == "packed")
            layoutItemsPacked(itemPointers, aspect);
        else if (m == "packed2")
            layoutItemsPacked2(itemPointers, aspect);
        else if (m == "row")
            layoutItemsInGrid(itemPointers, aspect, 0, 1);
        else if (m == "column")
            layoutItemsInGrid(itemPointers, aspect, 1, 0);
        else if (m == "grid")
            layoutItemsInGrid(itemPointers, aspect, cols, rows);
        else
        {
            setTrans = false;
            doSpacing = false;
        }

        if (doSpacing)
        {
            float spacing = m_spacing->front();
            if (spacing < 1.0 && spacing > 0.0)
                scaleAllItems(itemPointers, spacing);
        }

        if (setTrans)
            setTransforms(infos, items);
    }

    void LayoutGroupIPNode::propertyChanged(const Property* p)
    {
        if (!isDeleting())
        {
            if (p == m_mode || p == m_spacing || p == m_gridRows
                || p == m_gridColumns)
            {
                requestLayout();
            }
            if (p == m_retimeToOutput || p == m_stackNode->outputFPSProperty())
            {
                rebuild();
                propagateRangeChange();
                if (p == m_stackNode->outputFPSProperty())
                    return;
            }
        }

        GroupIPNode::propertyChanged(p);
    }

    void LayoutGroupIPNode::inputImageStructureChanged(int index,
                                                       PropagateTarget target)
    {
        GroupIPNode::inputImageStructureChanged(index, target);

        const string& m = m_mode->front();
        if (!isDeleting()
            && (m == "packed" || m == "packed2" || m == "column" || m == "row"
                || m == "grid"))
        {
            requestLayout();
        }
    }

    void LayoutGroupIPNode::setInputs(const IPNodes& newInputs)
    {
        HOP_PROF_FUNC();
        if (isDeleting())
        {
            IPNode::setInputs(newInputs);
        }
        else
        {
            {
                HOP_PROF("setInputsWithReordering");
                setInputsWithReordering(newInputs, m_stackNode);
            }
            {
                HOP_PROF("layout");
                requestLayout();
            }
        }
    }

    void LayoutGroupIPNode::rebuild(int inputIndex)
    {
        if (!isDeleting())
        {
            graph()->beginGraphEdit();
            IPNodes cachedInputs = inputs();
            if (inputIndex >= 0)
                setInputsWithReordering(cachedInputs, m_stackNode, inputIndex);
            else
                setInputs(cachedInputs);
            graph()->endGraphEdit();
        }
    }

    void LayoutGroupIPNode::readCompleted(const string& t, unsigned int v)
    {
        requestLayout();
        GroupIPNode::readCompleted(t, v);
    }

    IPImage* LayoutGroupIPNode::evaluate(const Context& context)
    {
        layoutIfRequested();
        return GroupIPNode::evaluate(context);
    }

    void LayoutGroupIPNode::inputMediaChanged(IPNode* srcNode, int srcOutIndex,
                                              PropagateTarget target)
    {
        IPNode::inputMediaChanged(srcNode, srcOutIndex, target);

        auto inputIndex = mapToInputIndex(srcNode, srcOutIndex);
        if (inputIndex < 0)
        {
            return;
        }

        rebuild(inputIndex);
    }
} // namespace IPCore
