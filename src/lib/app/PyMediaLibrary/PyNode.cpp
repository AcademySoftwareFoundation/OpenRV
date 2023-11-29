//
//  Copyright (c) 2023 Autodesk, Inc. All Rights Reserved.
//  SPDX-License-Identifier: Apache-2.0
//

#include <PyMediaLibrary/PyNode.h>
#include <PyMediaLibrary/PyMediaLibrary.h>

#include <TwkMediaLibrary/Library.h>

namespace TwkMediaLibrary
{

  class PyMediaLibrary;

  std::string nameFromPyNodeType( PyNodeType t )
  {
    switch( t )
    {
      default:
      case PyNoType:
        return "PyNoType";
      case PyMediaType:
        return "PyMediaType";
      case PyRootType:
        return "PyRootType";
    }

    return "";
  }

  PyNode::PyNode( Library* lib, PyNode* parent, std::string name,
                  PyNodeType type )
      : Node( lib ),
        m_type( type ),
        m_name( std::move( name ) ),
        m_parent( parent )
  {
    if( m_parent ) m_parent->addChild( this );
  }

  void PyNode::addChild( PyNode* child )
  {
    m_children.push_back( child );
  }

  std::string PyNode::typeName() const
  {
    return nameFromPyNodeType( m_type );
  }

  std::string PyNode::name() const
  {
    return m_name;
  }

  const Node* PyNode::parent() const
  {
    return m_parent;
  }

  size_t PyNode::numChildren() const
  {
    return m_children.size();
  }

  const Node* PyNode::child( size_t index ) const
  {
    return index < m_children.size() ? m_children[index] : nullptr;
  }

  void PyNode::setName( const std::string& n )
  {
    m_name = n;
  }

}  // namespace TwkMediaLibrary