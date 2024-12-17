//
//  Copyright (c) 2013 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __RvCommon__GLSLSyntaxHighlighter__h__
#define __RvCommon__GLSLSyntaxHighlighter__h__
#include <QtGui/QSyntaxHighlighter>
#include <QtCore/QHash>
#include <QtGui/QTextCharFormat>
#include <iostream>

class QTextDocument;

namespace Rv
{

    class GLSLSyntaxHighlighter : public QSyntaxHighlighter
    {
        Q_OBJECT

    public:
        struct HighlightingRule
        {
            QRegExp pattern;
            QTextCharFormat format;
        };

        GLSLSyntaxHighlighter(QTextDocument* parent = 0);

    protected:
        void highlightBlock(const QString& text);

    private:
        QVector<HighlightingRule> m_highlightingRules;
        QRegExp m_commentStartExpression;
        QRegExp m_commentEndExpression;
        QTextCharFormat m_typeFormat;
        QTextCharFormat m_keywordFormat;
        QTextCharFormat m_builtInFormat;
        QTextCharFormat m_imageFormat;
        QTextCharFormat m_commentFormat;
        QTextCharFormat m_quotationFormat;
        QTextCharFormat m_functionFormat;
        QTextCharFormat m_preProcessorFormat;
    };

} // namespace Rv

#endif // __RvCommon__GLSLSyntaxHighlighter__h__
