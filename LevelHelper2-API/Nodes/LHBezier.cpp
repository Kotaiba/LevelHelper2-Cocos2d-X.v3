//
//  LHBezier.cpp
//  LevelHelper2-Cocos2d-X-v3
//
//  Created by Bogdan Vladu on 31/03/14.
//  Copyright (c) 2014 GameDevHelper.com. All rights reserved.
//

#include "LHBezier.h"
#include "LHDictionary.h"
#include "LHScene.h"
#include "LHDevice.h"


//! a Point with a vertex point, a tex coord point and a color 4B
typedef struct _LH_V2F_C4B
{
	//! vertices (2F)
	Vertex2F		vertices;
	//! colors (4B)
	Color4B		colors;
} lhV2F_C4B;


static float MAX_BEZIER_STEPS = 24.0f;


Point LHPointOnCurve(const Point& p1,
                     const Point& p2,
                     const Point& p3,
                     const Point& p4,
                     const float& t)
{
	float var1, var2, var3;
    Point vPoint;
    
    var1 = 1 - t;
    var2 = var1 * var1 * var1;
    var3 = t * t * t;
    vPoint.x = var2*p1.x + 3*t*var1*var1*p2.x + 3*t*t*var1*p3.x + var3*p4.x;
    vPoint.y = var2*p1.y + 3*t*var1*var1*p2.y + 3*t*t*var1*p3.y + var3*p4.y;
    return(vPoint);
}

LHBezier::LHBezier()
{
}

LHBezier::~LHBezier()
{
    
}

LHBezier* LHBezier::bezierNodeWithDictionary(LHDictionary* dict, Node* prnt)
{
    LHBezier *ret = new LHBezier();
    if (ret && ret->initWithDictionary(dict, prnt))
    {
        ret->autorelease();
        return ret;
    }
    else
    {
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
}

bool LHBezier::initWithDictionary(LHDictionary* dict, Node* prnt)
{
    if(DrawNode::init())
    {
        _physicsBody = NULL;
        
        loadGenericInfoFromDictionary(dict);

        float alpha = dict->floatForKey("alpha");
        this->setOpacity(alpha);
        this->setRotation(dict->floatForKey("rotation"));
        this->setZOrder(dict->floatForKey("zOrder"));
        
        clear();
        
        Color3B overlay = dict->colorForKey("colorOverlay");
        
        Color4F colorOverlay(overlay.r/255.0f, overlay.g/255.0f, overlay.b/255.0f, alpha/255.0f);
        
        LHArray* points = dict->arrayForKey("points");
        bool closed     = dict->boolForKey("closed");
        
        Point* prevValue = nullptr;
        LHDictionary* previousPointDict = nullptr;
        for(int i = 0; i < points->count(); ++i)
        {
            LHDictionary* pointDict = points->dictAtIndex(i);

            if(previousPointDict != nullptr)
            {
                Point control1 = previousPointDict->pointForKey("ctrl2");
                if(!previousPointDict->boolForKey("hasCtrl2")){
                    control1 = previousPointDict->pointForKey("mainPt");
                }
                
                Point control2 = pointDict->pointForKey("ctrl1");
                if(!pointDict->boolForKey("hasCtrl1")){
                    control2 = pointDict->pointForKey("mainPt");
                }
                
                Point vPoint = {0.0f, 0.0f};
                for(float t = 0.0; t <= (1 + (1.0f / MAX_BEZIER_STEPS)); t += 1.0f / MAX_BEZIER_STEPS)
                {
                    vPoint = LHPointOnCurve(previousPointDict->pointForKey("mainPt"),
                                            control1,
                                            control2,
                                            pointDict->pointForKey("mainPt"),
                                            t);
                    
                    Point pt(vPoint.x, -vPoint.y);
                    
                    
                    if(prevValue){
                        Point prevPt = Point(prevValue->x, prevValue->y);
                        drawSegment(prevPt, pt, 1, colorOverlay);
                    }
                    
                    CC_SAFE_DELETE(prevValue);
                    prevValue = new Point(pt);
                    _linePoints.push_back(pt);
                }
            }
            previousPointDict = pointDict;
        }
        if(closed){
            if(points->count() > 1)
            {
                LHDictionary* ptDict = points->dictAtIndex(0);
                
                Point control1 = previousPointDict->pointForKey("ctrl2");
                if(!previousPointDict->boolForKey("hasCtrl2")){
                    control1 = previousPointDict->pointForKey("mainPt");
                }
                
                Point control2 = ptDict->pointForKey("ctrl1");
                if(!ptDict->boolForKey("hasCtrl1")){
                    control2 = ptDict->pointForKey("mainPt");
                }
                
                Point vPoint = {0.0f, 0.0f};
                for(float t = 0; t <= (1 + (1.0f / MAX_BEZIER_STEPS)); t += 1.0f / MAX_BEZIER_STEPS)
                {
                    vPoint = LHPointOnCurve(previousPointDict->pointForKey("mainPt"),
                                            control1,
                                            control2,
                                            ptDict->pointForKey("mainPt"),
                                            t);
                    
                    Point pt(vPoint.x, -vPoint.y);
                    if(prevValue){
                        Point prevPt = Point(prevValue->x, prevValue->y);
                        drawSegment(prevPt, pt, 1, colorOverlay);
                    }
                    CC_SAFE_DELETE(prevValue);
                    prevValue = new Point(pt);
                    _linePoints.push_back(pt);
                }
            }
        }
        CC_SAFE_DELETE(prevValue);
        
        //physics body needs to be created before adding this node to the parent
        loadPhysicsFromDictionary(dict->dictForKey("nodePhysics"), (LHScene*)prnt->getScene());
        prnt->addChild(this);

        
        Point scl = dict->pointForKey("scale");
        this->setScaleX(scl.x);
        this->setScaleY(scl.y);
        
        
        Point anchor = dict->pointForKey("anchor");
        anchor.y = 1.0f - anchor.y;
        this->setAnchorPoint(anchor);

        Point unitPos   = dict->pointForKey("generalPosition");
        Point pos       = LHScene::positionForNode(this, unitPos);
        
        LHDictionary* devPositions = dict->dictForKey("devicePositions");
        if(devPositions)
        {
            std::string unitPosStr = LHDevice::devicePosition(devPositions, LH_SCREEN_RESOLUTION);
            
            if(unitPosStr.length()>0){
                Point unitPos = PointFromString(unitPosStr);
                pos = LHScene::positionForNode(this, unitPos);
            }
        }
        this->setPosition(pos);
        
        
        LHArray* childrenInfo = dict->arrayForKey("children");
        if(childrenInfo)
        {
            for(int i = 0; i < childrenInfo->count(); ++i)
            {
                LHDictionary* childInfo = childrenInfo->dictAtIndex(i);
                
                Node* node = LHScene::createLHNodeWithDictionary(childInfo, this);
#pragma unused (node)
            }
        }
        
        createAnimationsFromDictionary(dict);
        
        return true;
    }
    
    return false;
}

std::vector<Point> LHBezier::linePoints(){
    return _linePoints;
}

void LHBezier::visit(Renderer *renderer, const kmMat4& parentTransform, bool parentTransformUpdated)
{
    visitActiveAnimation();
    DrawNode::visit(renderer, parentTransform, parentTransformUpdated);
}
