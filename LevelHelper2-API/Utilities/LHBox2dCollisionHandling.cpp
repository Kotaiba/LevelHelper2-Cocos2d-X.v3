//
//  LHBox2dCollisionHandling.cpp
//  LevelHelper2-Cocos2d-X v3
//
//  Created by Bogdan Vladu on 06/07/14.
//  Copyright (c) 2014 VLADU BOGDAN DANIEL PFA. All rights reserved.
//

#import "LHBox2dCollisionHandling.h"
#import "LHScene.h"
#import "LHGameWorldNode.h"
#import "LHConfig.h"
#import "LHUtils.h"
#if LH_USE_BOX2D

#include "Box2d/Box2d.h"

class LHContactListenerPimpl : public b2ContactListener
{
public:
    void* nodeObject;
    void (*preSolveSelector)(void*,
                             b2Contact* contact,
                             const b2Manifold* oldManifold);
    
    void (*postSolveSelector)(void*,
                              b2Contact* contact,
                              const b2ContactImpulse* impulse);
    
    void (*beginSolveSelector)(void*,
                               b2Contact* contact);

    void (*endSolveSelector)(void*,
                             b2Contact* contact);

    
    LHContactListenerPimpl(){};
    ~LHContactListenerPimpl(){};
	
    /// Called when two fixtures begin to touch.
	virtual void BeginContact(b2Contact* contact);
	/// Called when two fixtures cease to touch.
	virtual void EndContact(b2Contact* contact);

	/// This is called after a contact is updated. This allows you to inspect a
	/// contact before it goes to the solver. If you are careful, you can modify the
	/// contact manifold (e.g. disable contact).
	/// A copy of the old manifold is provided so that you can detect changes.
	/// Note: this is called only for awake bodies.
	/// Note: this is called even when the number of contact points is zero.
	/// Note: this is not called for sensors.
	/// Note: if you set the number of contact points to zero, you will not
	/// get an EndContact callback. However, you may get a BeginContact callback
	/// the next step.
	virtual void PreSolve(b2Contact* contact, const b2Manifold* oldManifold);
    
	/// This lets you inspect a contact after the solver is finished. This is useful
	/// for inspecting impulses.
	/// Note: the contact manifold does not include time of impact impulses, which can be
	/// arbitrarily large if the sub-step is small. Hence the impulse is provided explicitly
	/// in a separate data structure.
	/// Note: this is only called for contacts that are touching, solid, and awake.
	virtual void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse);
    
};
void LHContactListenerPimpl::BeginContact(b2Contact* contact){
    if(contact->GetFixtureA() != NULL && contact->GetFixtureB() != NULL)
        (*beginSolveSelector)(nodeObject,contact);
}
void LHContactListenerPimpl::EndContact(b2Contact* contact){
    if(contact->GetFixtureA() != NULL && contact->GetFixtureB() != NULL)
        (*endSolveSelector)(nodeObject,contact);
}
void LHContactListenerPimpl::PreSolve(b2Contact* contact,
                                      const b2Manifold* oldManifold){
    if(contact->GetFixtureA() != NULL && contact->GetFixtureB() != NULL)
        (*preSolveSelector)( nodeObject, contact, oldManifold);
}
void LHContactListenerPimpl::PostSolve(b2Contact* contact,
                                       const b2ContactImpulse* impulse){
    if(contact->GetFixtureA() != NULL && contact->GetFixtureB() != NULL)
        (*postSolveSelector)(nodeObject, contact, impulse);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void lhContactPreSolveCaller(void* object,
                             b2Contact* contact,
                             const b2Manifold* oldManifold);
void lhContactPostSolveCaller(void* object,
                              b2Contact* contact,
                              const b2ContactImpulse* impulse);
void lhContactBeginContactCaller(void* object,
                                 b2Contact* contact);
void lhContactEndContactCaller(void* object,
                               b2Contact* contact);
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//@interface LHActiveContact : NSObject
//{
//    __unsafe_unretained SKNode* nodeA;
//    __unsafe_unretained SKNode* nodeB;
//    CGPoint contactPoint;
//    BOOL _disabled;
//}
//+(instancetype)activeContactWithA:(SKNode*)a
//                                b:(SKNode*)b
//                         disabled:(BOOL)disabled
//                     contactPoint:(CGPoint)pt;
//
//-(SKNode*)nodeA;
//-(SKNode*)nodeB;
//-(CGPoint)contactPoint;
//-(BOOL)disabled;
//@end
//@implementation LHActiveContact
//
//-(id)initActiveContactWithA:(SKNode*)a
//                          b:(SKNode*)b
//                   disabled:(BOOL)disabled
//               contactPoint:(CGPoint)pt
//{
//    if(self = [super init])
//    {
//        nodeA = a;
//        nodeB = b;
//        _disabled = disabled;
//        contactPoint = pt;
//    }
//    return self;
//}
//+(instancetype)activeContactWithA:(SKNode *)a
//                                b:(SKNode *)b
//                         disabled:(BOOL)disabled
//                     contactPoint:(CGPoint)pt
//{
//    return LH_AUTORELEASED([[self alloc] initActiveContactWithA:a
//                                                              b:b
//                                                       disabled:disabled
//                                                   contactPoint:pt]);
//}
//-(SKNode*)nodeA{
//    return nodeA;
//}
//-(SKNode*)nodeB{
//    return nodeB;
//}
//-(BOOL)disabled{
//    return _disabled;
//}
//-(CGPoint)contactPoint{
//    return contactPoint;
//}
//@end
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
LHBox2dCollisionHandling::~LHBox2dCollisionHandling(){
    _scene->getBox2dWorld()->SetContactListener(NULL);
    _scene = NULL;
    CC_SAFE_DELETE(_b2Listener);
}

LHBox2dCollisionHandling::LHBox2dCollisionHandling(LHScene* scene)
{
    _scene = scene;
    
    _b2Listener = new LHContactListenerPimpl();
    _b2Listener->nodeObject = this;
    
    _scene->getBox2dWorld()->SetContactListener(_b2Listener);

    _b2Listener->preSolveSelector   = &lhContactPreSolveCaller;
    _b2Listener->postSolveSelector  = &lhContactPostSolveCaller;
    _b2Listener->beginSolveSelector = &lhContactBeginContactCaller;
    _b2Listener->endSolveSelector   = &lhContactEndContactCaller;
}

b2Fixture* LHBox2dCollisionHandling::getFixtureAFromContact(b2Contact* contact){
    return contact->GetFixtureA();
}

b2Fixture* LHBox2dCollisionHandling::getFixtureBFromContact(b2Contact* contact){
    return contact->GetFixtureB();
}

b2Body* LHBox2dCollisionHandling::getBodyAFromContact(b2Contact* contact)
{
    b2Fixture* fixtureA = contact->GetFixtureA();
    if(!fixtureA)return NULL;
    return fixtureA->GetBody();
}

b2Body* LHBox2dCollisionHandling::getBodyBFromContact(b2Contact* contact)
{
    b2Fixture* fixtureB = contact->GetFixtureB();
    if(!fixtureB)return NULL;
    return fixtureB->GetBody();
}

Node* LHBox2dCollisionHandling::getNodeAFromContact(b2Contact* contact){
    b2Body* bodyA = this->getBodyAFromContact(contact);
    if(!bodyA)return NULL;
    return (Node*)bodyA->GetUserData();
}

Node* LHBox2dCollisionHandling::getNodeBFromContact(b2Contact* contact){
    b2Body* bodyB = this->getBodyBFromContact(contact);
    if(!bodyB)return NULL;
    return (Node*)bodyB->GetUserData();
}
Point LHBox2dCollisionHandling::getPointFromContact(b2Contact* contact)
{
    b2WorldManifold worldManifold;
    contact->GetWorldManifold(&worldManifold);
    b2Vec2 worldPt = worldManifold.points[0];
    return _scene->pointFromMeters(worldPt);
}

void LHBox2dCollisionHandling::preSolve(b2Contact* contact, const b2Manifold* oldManifold)
{
    Node* nodeA = this->getNodeAFromContact(contact);
    Node* nodeB = this->getNodeBFromContact(contact);
    if(!nodeA || !nodeB)return;
    
    //at this point ask the scene if we should disable this contact
    bool shouldDisable = _scene->shouldDisableContactBetweenNodes(nodeA, nodeB);

    if(shouldDisable){
        contact->SetEnabled(false);
    }
    //cancel collision here - if needed
}
void LHBox2dCollisionHandling::postSolve(b2Contact* contact, const b2ContactImpulse* contactImpulse)
{
    Node* nodeA = this->getNodeAFromContact(contact);
    Node* nodeB = this->getNodeBFromContact(contact);
    if(!nodeA || !nodeB)return;

    float impulse = 0;
    if(contactImpulse->count > 0)
    {
        impulse = contactImpulse->normalImpulses[0];
    }
    _scene->didBeginContactBetweenNodes(nodeA, nodeB, this->getPointFromContact(contact), impulse);
    //at this point send the info to the scene
}
void LHBox2dCollisionHandling::beginContact(b2Contact* contact)
{
    Node* nodeA = this->getNodeAFromContact(contact);
    Node* nodeB = this->getNodeBFromContact(contact);
    if(!nodeA || !nodeB)return;
    
    //called for sensors
    _scene->didBeginContactBetweenNodes(nodeA, nodeB, this->getPointFromContact(contact), 0);
    
//    [_activeContacts addObject:[LHActiveContact activeContactWithA:nodeA
//                                                                 b:nodeB
//                                                          disabled:NO
//                                                      contactPoint:[self getPointFromContact:contact]]];
}
void LHBox2dCollisionHandling::endContact(b2Contact* contact)
{
    Node* nodeA = this->getNodeAFromContact(contact);
    Node* nodeB = this->getNodeBFromContact(contact);
    if(!nodeA || !nodeB)return;

    _scene->didEndContactBetweenNodes(nodeA, nodeB);
}


void lhContactPreSolveCaller(void* object,
                             b2Contact* contact,
                             const b2Manifold* oldManifold)
{
    LHBox2dCollisionHandling* collision = (LHBox2dCollisionHandling*)object;
    collision->preSolve(contact, oldManifold);
}

void lhContactPostSolveCaller(void* object,
                              b2Contact* contact,
                              const b2ContactImpulse* impulse)
{
    LHBox2dCollisionHandling* collision = (LHBox2dCollisionHandling*)object;
    collision->postSolve(contact, impulse);
}
void lhContactBeginContactCaller(void* object,
                                 b2Contact* contact)
{
    LHBox2dCollisionHandling* collision = (LHBox2dCollisionHandling*)object;
    collision->beginContact(contact);
}
void lhContactEndContactCaller(void* object,
                               b2Contact* contact)
{
    LHBox2dCollisionHandling* collision = (LHBox2dCollisionHandling*)object;
    collision->endContact(contact);
}

#endif