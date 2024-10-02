//
//  Copyright (c) 2013 Tweak Software. 
//  All rights reserved.
//  
//  SPDX-License-Identifier: Apache-2.0
//  
//
#ifndef __RvCommon__GLSLSyntaxHighlighter__h__
#define __RvCommon__GLSLSyntaxHighlighter__h__
#include <QRegularExpression>
#include <QtGui/QSyntaxHighlighter>
#include <QtCore/QHash>
#include <QtGui/QTextCharFormat>
#include <iostream>

class QTextDocument;

namespace Rv {

class GLSLSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
    
  public:

    struct HighlightingRule
    {
        QRegularExpression  pattern;
        QTextCharFormat     format;
    };

    GLSLSyntaxHighlighter(QTextDocument *parent = 0);
    
  protected:
    void highlightBlock(const QString &text);
    
  private:

    QVector<HighlightingRule> m_highlightingRules;
    QRegularExpression        m_commentStartExpression;
    QRegularExpression        m_commentEndExpression;
    QTextCharFormat           m_typeFormat;
    QTextCharFormat           m_keywordFormat;
    QTextCharFormat           m_builtInFormat;
    QTextCharFormat           m_imageFormat;
    QTextCharFormat           m_commentFormat;
    QTextCharFormat           m_quotationFormat;
    QTextCharFormat           m_functionFormat;
    QTextCharFormat           m_preProcessorFormat;
};

} // Rv

#endif // __RvCommon__GLSLSyntaxHighlighter__h__
