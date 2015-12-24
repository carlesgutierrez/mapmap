/*
 * Shape.cpp
 *
 * (c) 2013 Sofian Audry -- info(@)sofianaudry(.)com
 * (c) 2013 Alexandre Quessy -- alexandre(@)quessy(.)net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ShapeGraphicsItem.h"

#include "MainWindow.h"

ShapeGraphicsItem::ShapeGraphicsItem(Mapping::ptr mapping, bool output)
  : _mapping(mapping), _output(output)
{
  _shape = output ? getMapping()->getShape() : getMapping()->getInputShape();
}

MapperGLCanvas* ShapeGraphicsItem::getCanvas() const
{
  MainWindow* win = MainWindow::instance();
  return isOutput() ? win->getDestinationCanvas() : win->getSourceCanvas();
}

bool ShapeGraphicsItem::isMappingCurrent() const {
  return MainWindow::instance()->getCurrentMappingId() == getMapping()->getId();
}

void ShapeGraphicsItem::paint(QPainter *painter,
                              const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  Q_UNUSED(widget);

  // Sync depth of figure with that of mapping (for layered output).
  if (isOutput())
    setZValue(getMapping()->getDepth());

  // Paint if visible.
  if (isMappingVisible())
  {
    // Paint whatever needs to be painted.
    _prePaint(painter, option);
    _doPaint(painter, option);
    _postPaint(painter, option);
  }
}

QPen ShapeGraphicsItem::getRescaledShapeStroke(bool innerStroke)
{
  return QPen(QBrush(MM::CONTROL_COLOR), (innerStroke ? MM::SHAPE_INNER_STROKE_WIDTH : MM::SHAPE_STROKE_WIDTH) / getCanvas()->getZoomFactor());
}

//void VertexGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent * event)
//{
//  ShapeGraphicsItem* shapeParent = static_cast<ShapeGraphicsItem*>(parentItem());
//  if (!shapeParent->isMappingVisible())
//  {
//    // Prevent mouse grabbing.
//    event->ignore();
//  }
//  else
//  {
//    if (shapeParent->isOutput())
//    {
//      QGraphicsItem::mousePressEvent(event);
//      if (event->button() == Qt::LeftButton)
//      {
//        MainWindow::instance()->setCurrentMapping(shapeParent->getMapping()->getId());
//      }
//    }
//    else
//    {
//      if (shapeParent->isMappingCurrent())
//        QGraphicsItem::mousePressEvent(event);
//      else
//        event->ignore(); // prevent mousegrabbing on non-current mapping
//    }
//  }
//}
//
//void VertexGraphicsItem::paint(QPainter *painter,
//    const QStyleOptionGraphicsItem *option,
//    QWidget* widget)
//{
//  Q_UNUSED(widget);
////  if (MainWindow::instance()->displayControls())
////  {
////    ShapeGraphicsItem* shapeParent = static_cast<ShapeGraphicsItem*>(parentItem());
////    if (shapeParent->isMappingVisible() &&
////        shapeParent->isMappingCurrent())
////    {
////      qreal zoomFactor = 1.0 / shapeParent->getCanvas()->getZoomFactor();
////      resetMatrix();
////      scale(zoomFactor, zoomFactor);
////      Util::drawControlsVertex(painter, QPointF(0,0), (option->state & QStyle::State_Selected), MM::VERTEX_SELECT_RADIUS);
////    }
////  }
//}

void ColorGraphicsItem::_prePaint(QPainter *painter,
                                  const QStyleOptionGraphicsItem *option)
{
  Color* color = static_cast<Color*>(getMapping()->getPaint().data());
  Q_ASSERT(color);

  painter->setPen(Qt::NoPen);

  // Set brush.
  QColor col = color->getColor();
  col.setAlphaF(getMapping()->getOpacity());
  painter->setBrush(col);
}

PolygonColorGraphicsItem::PolygonColorGraphicsItem(Mapping::ptr mapping, bool output)
  : ColorGraphicsItem(mapping, output) {
  _controlPainter.reset(new PolygonControlPainter(this));
}

QPainterPath PolygonColorGraphicsItem::shape() const
{
  QPainterPath path;
  Polygon* poly = static_cast<Polygon*>(_shape.data());
  Q_ASSERT(poly);
  path.addPolygon(poly->toPolygon());
  return mapFromScene(path);
}

void PolygonColorGraphicsItem::_doPaint(QPainter *painter,
                                        const QStyleOptionGraphicsItem *option)
{
  Q_UNUSED(option);
  Polygon* poly = static_cast<Polygon*>(_shape.data());
  Q_ASSERT(poly);
  painter->drawPolygon(mapFromScene(poly->toPolygon()));
}

EllipseColorGraphicsItem::EllipseColorGraphicsItem(Mapping::ptr mapping, bool output)
  : ColorGraphicsItem(mapping, output) {
    _controlPainter.reset(new EllipseControlPainter(this));
  }

QPainterPath EllipseColorGraphicsItem::shape() const
{
  // Create path for ellipse.
  QPainterPath path;
  Ellipse* ellipse = static_cast<Ellipse*>(_shape.data());
  Q_ASSERT(ellipse);
  QTransform transform;
  transform.translate(ellipse->getCenter().x(), ellipse->getCenter().y());
  transform.rotate(ellipse->getRotation());
  path.addEllipse(QPoint(0,0), ellipse->getHorizontalRadius(), ellipse->getVerticalRadius());
  return mapFromScene(transform.map(path));
}

void EllipseColorGraphicsItem::_doPaint(QPainter* painter,
                                        const QStyleOptionGraphicsItem* option)
{
  Q_UNUSED(option);
  // Just draw the path.
  painter->drawPath(shape());
}

TextureGraphicsItem::TextureGraphicsItem(Mapping::ptr mapping, bool output)
  : ShapeGraphicsItem(mapping, output)
{
  _textureMapping = qSharedPointerCast<TextureMapping>(mapping);
  Q_CHECK_PTR(_textureMapping);

  _texture = qSharedPointerCast<Texture>(_textureMapping.toStrongRef()->getPaint());
  Q_CHECK_PTR(_texture);

  _inputShape = qSharedPointerCast<MShape>(_textureMapping.toStrongRef()->getInputShape());
  Q_CHECK_PTR(_inputShape);
}

void TextureGraphicsItem::_doPaint(QPainter *painter,
                                   const QStyleOptionGraphicsItem *option)
{
  Q_UNUSED(option);
  // Perform the actual mapping (done by subclasses).
  if (isOutput())
    _doDrawOutput(painter);
  else
    _doDrawInput(painter);
}

void TextureGraphicsItem::_doDrawInput(QPainter* painter)
{
  Q_UNUSED(painter);
  if (isMappingCurrent())
  {
    // FIXME: Does this draw the quad counterclockwise?
    glBegin (GL_QUADS);
    {
      QRectF rect = mapFromScene(_texture.toStrongRef()->getRect()).boundingRect();

      Util::correctGlTexCoord(0, 0);
      glVertex3f (rect.x(), rect.y(), 0);

      Util::correctGlTexCoord(1, 0);
      glVertex3f (rect.x() + rect.width(), rect.y(), 0);

      Util::correctGlTexCoord(1, 1);
      glVertex3f (rect.x()+rect.width(), rect.y()+rect.height(), 0);

      Util::correctGlTexCoord(0, 1);
      glVertex3f (rect.x(), rect.y()+rect.height(), 0);
    }
    glEnd ();
  }
}

void TextureGraphicsItem::_prePaint(QPainter* painter,
                                    const QStyleOptionGraphicsItem *option)
{
  Q_UNUSED(option);
  painter->beginNativePainting();

  QSharedPointer<Texture> texture = _texture.toStrongRef();

  // Only works for similar shapes.
  // TODO:remettre
  //Q_ASSERT( _inputShape->nVertices() == outputShape->nVertices());

  // Project source texture and sent it to destination.
  texture->update();

  // Allow alpha blending.
  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Get texture.
  glEnable (GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, texture->getTextureId());

  // Copy bits to texture iff necessary.
  texture->lockMutex();
  if (texture->bitsHaveChanged())
  {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
        texture->getWidth(), texture->getHeight(), 0, GL_RGBA,
      GL_UNSIGNED_BYTE, texture->getBits());
  }
  texture->unlockMutex();

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // Set texture color (apply opacity).
  glColor4f(1.0f, 1.0f, 1.0f,
            isOutput() ? getMapping()->getOpacity() : getMapping()->getPaint()->getOpacity());
}

void TextureGraphicsItem::_postPaint(QPainter* painter,
                                     const QStyleOptionGraphicsItem *option)
{
  Q_UNUSED(option);

  glDisable(GL_TEXTURE_2D);

  painter->endNativePainting();
}

QPainterPath PolygonTextureGraphicsItem::shape() const
{
  QPainterPath path;
  Polygon* poly = static_cast<Polygon*>(_shape.data());
  Q_ASSERT(poly);
  path.addPolygon(poly->toPolygon());
  return mapFromScene(path);
}

QRectF PolygonTextureGraphicsItem::boundingRect() const {
  return shape().boundingRect();
}

void TriangleTextureGraphicsItem::_doDrawOutput(QPainter* painter)
{
  Q_UNUSED(painter);
  if (isOutput())
  {
    MShape::ptr inputShape = _inputShape.toStrongRef();
    glBegin(GL_TRIANGLES);
    {
      for (int i=0; i<inputShape->nVertices(); i++)
      {
        Util::setGlTexPoint(*_texture.toStrongRef(), inputShape->getVertex(i), mapFromScene(getShape()->getVertex(i)));
      }
    }
    glEnd();
  }
}

PolygonTextureGraphicsItem::PolygonTextureGraphicsItem(Mapping::ptr mapping, bool output) : TextureGraphicsItem(mapping, output) {
  _controlPainter.reset(new PolygonControlPainter(this));
}

void MeshTextureGraphicsItem::_doDrawOutput(QPainter* painter)
{
  Q_UNUSED(painter);
  if (isOutput())
  {
    QSharedPointer<Mesh> outputMesh = qSharedPointerCast<Mesh>(_shape);
    QSharedPointer<Mesh> inputMesh  = qSharedPointerCast<Mesh>(_inputShape);
    QVector<QVector<Quad> > outputQuads = outputMesh->getQuads2d();
    QVector<QVector<Quad> > inputQuads  = inputMesh->getQuads2d();
    for (int x = 0; x < outputMesh->nHorizontalQuads(); x++)
    {
      for (int y = 0; y < outputMesh->nVerticalQuads(); y++)
      {
        QSizeF size = mapFromScene(outputQuads[x][y].toPolygon()).boundingRect().size();
        float area = size.width() * size.height();
        _drawQuad(*_texture.toStrongRef(), inputQuads[x][y], outputQuads[x][y], area);
      }
    }
  }

}

MeshTextureGraphicsItem::MeshTextureGraphicsItem(Mapping::ptr mapping, bool output) : PolygonTextureGraphicsItem(mapping, output) {
  _controlPainter.reset(new MeshControlPainter(this));
}


void MeshTextureGraphicsItem::_drawQuad(const Texture& texture, const Quad& inputQuad, const Quad& outputQuad, float outputArea, float inputThreshod, float outputThreshold)
{
  QPointF oa = mapFromScene(outputQuad.getVertex(0));
  QPointF ob = mapFromScene(outputQuad.getVertex(1));
  QPointF oc = mapFromScene(outputQuad.getVertex(2));
  QPointF od = mapFromScene(outputQuad.getVertex(3));

  QPointF ia = inputQuad.getVertex(0);
  QPointF ib = inputQuad.getVertex(1);
  QPointF ic = inputQuad.getVertex(2);
  QPointF id = inputQuad.getVertex(3);

  // compute the dot products for the polygon
  float outputV1dotV2 = QPointF::dotProduct(oa-ob, oc-ob);
  float outputV3dotV4 = QPointF::dotProduct(oc-od, oa-od);
  float outputV1dotV4 = QPointF::dotProduct(oa-ob, oa-od);
  float outputV2dotV3 = QPointF::dotProduct(oc-ob, oc-od);

  // compute the dot products for the texture
  float inputV1dotV2  = QPointF::dotProduct(ia-ib, ic-ib);
  float inputV3dotV4  = QPointF::dotProduct(ic-id, ia-id);
  float inputV1dotV4  = QPointF::dotProduct(ia-ib, ia-id);
  float inputV2dotV3  = QPointF::dotProduct(ic-ib, ic-id);

  // Stopping criterion.
  if (outputArea < 200 ||
      (fabs(outputV1dotV2 - outputV3dotV4) < outputThreshold &&
       fabs(outputV1dotV4 - outputV2dotV3) < outputThreshold &&
       fabs(inputV1dotV2  - inputV3dotV4)  < inputThreshod &&
       fabs(inputV1dotV4  - inputV2dotV3)  < inputThreshod))
  {
    glBegin(GL_QUADS);
    for (int i = 0; i < outputQuad.nVertices(); i++)
    {
      Util::setGlTexPoint(texture, inputQuad.getVertex(i), mapFromScene(outputQuad.getVertex(i)));
    }
    glEnd();
  }
  else // subdivide
  {
    QList<Quad> inputSubQuads  = _split(inputQuad);
    QList<Quad> outputSubQuads = _split(outputQuad);
    for (int i = 0; i < inputSubQuads.size(); i++)
    {
      _drawQuad(texture, inputSubQuads[i], outputSubQuads[i], outputArea*0.25, inputThreshod, outputThreshold);
    }
  }
}

QList<Quad> MeshTextureGraphicsItem::_split(const Quad& quad)
{
  QList<Quad> quads;

  QPointF a = quad.getVertex(0);
  QPointF b = quad.getVertex(1);
  QPointF c = quad.getVertex(2);
  QPointF d = quad.getVertex(3);

  QPointF ab = (a + b) * 0.5f;
  QPointF bc = (b + c) * 0.5f;
  QPointF cd = (c + d) * 0.5f;
  QPointF ad = (a + d) * 0.5f;

  QPointF abcd = (ab + cd) * 0.5f;

  quads.append(Quad(a, ab, abcd, ad));
  quads.append(Quad(ab, b, bc, abcd));
  quads.append(Quad(abcd, bc, c, cd));
  quads.append(Quad(ad, abcd, cd, d));

  return quads;
}

EllipseTextureGraphicsItem::EllipseTextureGraphicsItem(Mapping::ptr mapping, bool output) : TextureGraphicsItem(mapping, output) {
  _controlPainter.reset(new EllipseControlPainter(this));
}

QPainterPath EllipseTextureGraphicsItem::shape() const
{
  // Create path for ellipse.
  QPainterPath path;
  Ellipse* ellipse = static_cast<Ellipse*>(_shape.data());
  Q_ASSERT(ellipse);
  QTransform transform;
  transform.translate(ellipse->getCenter().x(), ellipse->getCenter().y());
  transform.rotate(ellipse->getRotation());
  path.addEllipse(QPoint(0,0), ellipse->getHorizontalRadius(), ellipse->getVerticalRadius());
  return mapFromScene(transform.map(path));
}

QRectF EllipseTextureGraphicsItem::boundingRect() const
{
  return shape().boundingRect();
}

void EllipseTextureGraphicsItem::_doDrawOutput(QPainter* painter)
{
  Q_UNUSED(painter);
  // Get input and output ellipses.
  QSharedPointer<Ellipse> inputEllipse  = qSharedPointerCast<Ellipse>(_inputShape);
  QSharedPointer<Ellipse> outputEllipse = qSharedPointerCast<Ellipse>(_shape);
  QSharedPointer<Texture> texture = _texture.toStrongRef();

  // Start / end angle.
  //const float startAngle = 0;
  //const float endAngle   = 2*M_PI;

  //
  //float angle;
  QPointF currentInputPoint;
  QPointF prevInputPoint(0, 0);
  QPointF currentOutputPoint;
  QPointF prevOutputPoint(0, 0);

  // Input ellipse parameters.
  const QPointF& inputCenter         = inputEllipse->getCenter();
  const QPointF& inputControlCenter  = inputEllipse->getVertex(4);
  float    inputHorizRadius          = inputEllipse->getHorizontalRadius();
  float    inputVertRadius           = inputEllipse->getVerticalRadius();
  float    inputRotation             = inputEllipse->getRotationRadians();

  // Output ellipse parameters.
  const QPointF& outputCenter        = mapFromScene(outputEllipse->getCenter());
  const QPointF& outputControlCenter = mapFromScene(outputEllipse->getVertex(4));
  float    outputHorizRadius         = outputEllipse->getHorizontalRadius();
  float    outputVertRadius          = outputEllipse->getVerticalRadius();
  float    outputRotation            = outputEllipse->getRotationRadians();

  // Variation in angle at each step of the loop.
  const int N_TRIANGLES = 100;
  const float ANGLE_STEP = 2*M_PI/N_TRIANGLES;

  float circleAngle = 0;
  for (int i=0; i<=N_TRIANGLES; i++, circleAngle += ANGLE_STEP)
  {
    // Set next (current) points.
    _setPointOfEllipseAtAngle(currentInputPoint, inputCenter, inputHorizRadius, inputVertRadius, inputRotation, circleAngle);
    _setPointOfEllipseAtAngle(currentOutputPoint, outputCenter, outputHorizRadius, outputVertRadius, outputRotation, circleAngle);

    // We don't draw the first point.
    if (i > 0)
    {
      // Draw triangle.
      glBegin(GL_TRIANGLES);
      Util::setGlTexPoint(*texture, inputControlCenter, outputControlCenter);
      Util::setGlTexPoint(*texture, prevInputPoint,     prevOutputPoint);
      Util::setGlTexPoint(*texture, currentInputPoint,  currentOutputPoint);
      glEnd();
    }

    // Save point for next iteration.
    prevInputPoint.setX(currentInputPoint.x());
    prevInputPoint.setY(currentInputPoint.y());
    prevOutputPoint.setX(currentOutputPoint.x());
    prevOutputPoint.setY(currentOutputPoint.y());
  }
}

void EllipseTextureGraphicsItem::_setPointOfEllipseAtAngle(QPointF& point, const QPointF& center, float hRadius, float vRadius, float rotation, float circularAngle)
{
  float xCirc = sin(circularAngle) * hRadius;
  float yCirc = cos(circularAngle) * vRadius;
  float distance = sqrt( xCirc*xCirc + yCirc*yCirc );
  float angle    = atan2( xCirc, yCirc );
  rotation = 2*M_PI-rotation; // rotation needs to be inverted (CW <-> CCW)
  point.setX( sin(angle + rotation) * distance + center.x() );
  point.setY( cos(angle + rotation) * distance + center.y() );
}