#include "BVH.h"
#include <algorithm>
#include "DrawUtils.h"
#include <CGLA/Vec2f.h>
#include "MetaTube.h"
#include "MetaBall.h"
#include <assert.h>
#include "BvhUtils.h"

bool xLess(const AABB& aabb1, const AABB& aabb2){
	return 0.5*(aabb1.bottomCorner+aabb1.topCorner)[0] <= 0.5*(aabb2.bottomCorner+aabb2.topCorner)[0];
}
bool xMinLess(const AABB& aabb1, const AABB& aabb2){
	return aabb1.bottomCorner[0] <= aabb2.bottomCorner[0];
}
bool xMaxLess(const AABB& aabb1, const AABB& aabb2){
	return aabb1.topCorner[0] <= aabb2.topCorner[0];
}


struct MinLessComp {
	int axis;
  bool operator() (const AABB& aabb1, const AABB& aabb2){
	return 0.5*(aabb1.bottomCorner+aabb1.topCorner)[axis] <= 0.5*(aabb2.bottomCorner+aabb2.topCorner)[axis];
	}
};
struct MaxLessComp {
	int axis;
  bool operator() (const AABB& aabb1, const AABB& aabb2){
	return 0.5*(aabb1.bottomCorner+aabb1.topCorner)[axis] <= 0.5*(aabb2.bottomCorner+aabb2.topCorner)[axis];
	}
};


struct MinLessCompL {
	int axis;
	bool operator() (const AABBLite& aabb1, const AABBLite& aabb2){
	return 0.5*(aabb1.bottomCorner+aabb1.topCorner)[axis] <= 0.5*(aabb2.bottomCorner+aabb2.topCorner)[axis];
	}
};
struct MaxLessCompL {
	int axis;
  bool operator() (const AABBLite& aabb1, const AABBLite& aabb2){
	return 0.5*(aabb1.bottomCorner+aabb1.topCorner)[axis] <= 0.5*(aabb2.bottomCorner+aabb2.topCorner)[axis];
	}
};

Vec2f v3DTov2D2(Vec3f v3d,int axis){
	Vec2f v2d;
	switch(axis){ //Layout : splix (y,z), spliy (x,z), spliz (x,y)
		case 0:
			v2d = Vec2f(v3d[1],v3d[2]);
			break;
		case 1:
			v2d = Vec2f(v3d[0],v3d[2]);
			break;
		case 2:
			v2d = Vec2f(v3d[0],v3d[1]);
			break;
	}
	return v2d;
}


void splitESCTest2(int axis, AABB& ab1/*In out*/, AABB& ab2/*out*/){

}



void renderAABB(const AABB& aabb){
	drawBox(aabb.bottomCorner,aabb.topCorner);
}

int child1(int node){
	return 2*node + 1;
}
int child2(int node){
	return 2*node + 2;
}
int nodeDepth(int node){
	int depthFirstIndex = 0;
	int depth = 0;
	while( node > depthFirstIndex ){
		depth++;
		depthFirstIndex = child1(depthFirstIndex);
	}
	return depth;
}
Vec4f CompactBvh::nodeBottom(int node)const{
	assert(2*node < dataSize);
	return data[2*node];
}
Vec4f CompactBvh::nodeTop(int node)const{
	assert(2*node+1 < dataSize);
	return data[2*node+1];
}
void CompactBvh::setNodeBottom(int node, const Vec4f& value){
	assert(2*node < dataSize);
	data[2*node] = value;
}
void CompactBvh::setNodeTop(int node, const Vec4f& value){
	assert(2*node+1 < dataSize);
	data[2*node+1] = value;
}
void CompactBvh::setLeafPrimitives(int node, const Vec4f& spheres, const Vec4f& tubes){

}
void CompactBvh::setTags(int node, float tag){
	data[2*node][3] = tag;
	//data[2*node+1][3] = tag;
}

CompactBvh::CompactBvh(){
	dataSize = 3*powf(2,MAX_BVH_D+1)-1;
	data = new Vec4f[dataSize];
	
	for(int i = 0; i < dataSize; i++)
		data[i]=Vec4f(-1);	
	maxNbOfNonLeafNodes = powf(2,MAX_BVH_D+1)-1-powf(2,MAX_BVH_D);
	maxNbOfNodes = powf(2,MAX_BVH_D+1) - 1;
	nbOfLeafs = 0;
}
CompactBvh::~CompactBvh(){
	delete[] data;
}

void CompactBvh::clear(){
	for(int i = 0; i < dataSize; i++)
		data[i]=Vec4f(-1);
	nbOfLeafs = 0;
}
void CompactBvh::initialize(int node,const vector<AABBLite>& aabbs){
	//Find the AABB of aabbs
	float minx = BIG, miny = BIG, minz = BIG, maxx = -BIG, maxy = -BIG, maxz = -BIG;
	for(unsigned int i  = 0; i < aabbs.size(); i++){
		if(aabbs[i].bottomCorner[0] < minx)
			minx = aabbs[i].bottomCorner[0];
		if(aabbs[i].topCorner[0] > maxx)
			maxx = aabbs[i].topCorner[0];

		if(aabbs[i].bottomCorner[1] < miny)
			miny = aabbs[i].bottomCorner[1];
		if(aabbs[i].topCorner[1] > maxy)
			maxy = aabbs[i].topCorner[1];

		if(aabbs[i].bottomCorner[2] < minz)
			minz = aabbs[i].bottomCorner[2];
		if(aabbs[i].topCorner[2] > maxz)
			maxz = aabbs[i].topCorner[2];
	}
	this->setNodeBottom(node,Vec4f(minx,miny,minz,-1));
	this->setNodeTop(node,Vec4f(maxx,maxy,maxz,-1));

	//If stoping criteria not reached
	if(  node < maxNbOfNonLeafNodes && aabbs.size()>1){ //this->aabb.topCorner[0] - this->aabb.bottomCorner[0] > 0.1 &&
	
		//Decide in which direction to split
		Vec4f bottom = nodeBottom(node);
		Vec4f top = nodeTop(node);
		float xSise = (top[0]-bottom[0]);
		float ySise = (top[1]-bottom[1]);
		float zSise = (top[2]-bottom[2]);

		int axis = 0;
		if( xSise < ySise){
			if(ySise < zSise){
				axis = 2;//splitz 
			}else{
				axis = 1;//splity
			}
		}else if(xSise < zSise){
			axis = 2;//splitz
		}else{
			axis = 0;//splitx
		}

		MinLessCompL mincomp;
		mincomp.axis = axis;
		MaxLessCompL maxcomp;
		maxcomp.axis = axis;

		vector<AABBLite> sortedAABBS;
		sortedAABBS.insert(sortedAABBS.begin(),aabbs.begin(),aabbs.end());
		//sort the aabbs on the x-axis according to the low corner
		std::sort(sortedAABBS.begin(),sortedAABBS.end(),mincomp);
		int mid = aabbs.size()/2; 

		vector<AABBLite> aabb1s;
		vector<AABBLite> aabb2s;

		aabb1s.insert(aabb1s.begin(),sortedAABBS.begin(),sortedAABBS.begin()+mid);
		aabb2s.insert(aabb2s.begin(),sortedAABBS.begin()+mid,sortedAABBS.end());

	
		for(int i = 0; i < aabb2s.size(); i++){
		}

		//Initialize the childs
		this->initialize(child1(node),aabb1s);
		this->initialize(child2(node),aabb2s);

	}else{
		Vec4f spheres(-1);
		Vec4f tubes(-1);

		MetaPrimitive* prim = aabbs[0].primitive;

		if(prim->getType() == "MetaTube"){
			MetaTube* tube = static_cast<MetaTube*>(prim);
			tubes[0] = tube->id; 
		}else if(prim->getType() == "MetaBall"){
			Metaball* ball = static_cast<Metaball*>(prim);
			spheres[0] = ball->id;
		}
		int primsLocation = nbOfLeafs + maxNbOfNodes;
		setTags(node,primsLocation);

		setNodeBottom(primsLocation,spheres);
		setNodeTop(primsLocation,tubes);

		nbOfLeafs++;
	}

}

float pointToBoxSquareDist(const Vec3f& point, const Vec3f& bottom, const Vec3f& top){
	//center of sphere in box axis
	Vec3f c = point - 0.5f*(bottom+top);

	//Box halfedges
	Vec3f a = 0.5f*(top-bottom);

	float s, d = 0; 

	//find the square of the distance
	//from the sphere to the box
	for( int i=0 ; i<3 ; i++ ){
		if( abs(c[i]) > a[i] ){
			s = abs(c[i]) - a[i];
			d += s*s; 
		}
	}

	return d;
}

bool CompactBvh::Intersects(MetaPrimitive* prim, int node){
	Vec3f bottom = Vec3f(nodeBottom(node));
	Vec3f top = Vec3f(nodeTop(node));
		
	if(prim->getType() == "MetaTube"){
		MetaTube* tube = static_cast<MetaTube*>(prim);

		float r = tube->r1;
		float d = pointToBoxSquareDist(tube->worldP1,bottom,top);

		if( d < r*r )
			return true;

		r = tube->r2;
		d = pointToBoxSquareDist(tube->worldP2,bottom,top);

		if( d < r*r )
			return true;
 
	}else if(prim->getType() == "MetaBall"){
		Metaball* ball = static_cast<Metaball*>(prim);
		float r = ball->r;
		float d = pointToBoxSquareDist(ball->worldC,bottom,top);

		if( d < r*r )
			return true;
	}

	return false;
}

bool CompactBvh::Intersects(MetaPrimitive* prim,const AABBLite& bbox){
	Vec3f bottom = bbox.bottomCorner;
	Vec3f top = bbox.topCorner;
		
	if(prim->getType() == "MetaTube"){
		MetaTube* tube = static_cast<MetaTube*>(prim);

		float r = tube->r1;
		float d = pointToBoxSquareDist(tube->worldP1,bottom,top);

		if( d < r*r )
			return true;

		r = tube->r2;
		d = pointToBoxSquareDist(tube->worldP2,bottom,top);

		if( d < r*r )
			return true;
 
	}else if(prim->getType() == "MetaBall"){
		Metaball* ball = static_cast<Metaball*>(prim);
		float r = ball->r;
		float d = pointToBoxSquareDist(ball->worldC,bottom,top);

		if( d < r*r )
			return true;
	}

	return false;
}

void CompactBvh::initializeSplit(int node,const vector<AABBLite>& aabbs){
	//Find the AABB of aabbs
	float minx = BIG, miny = BIG, minz = BIG, maxx = -BIG, maxy = -BIG, maxz = -BIG;
	for(unsigned int i  = 0; i < aabbs.size(); i++){
		if(aabbs[i].bottomCorner[0] < minx)
			minx = aabbs[i].bottomCorner[0];
		if(aabbs[i].topCorner[0] > maxx)
			maxx = aabbs[i].topCorner[0];

		if(aabbs[i].bottomCorner[1] < miny)
			miny = aabbs[i].bottomCorner[1];
		if(aabbs[i].topCorner[1] > maxy)
			maxy = aabbs[i].topCorner[1];

		if(aabbs[i].bottomCorner[2] < minz)
			minz = aabbs[i].bottomCorner[2];
		if(aabbs[i].topCorner[2] > maxz)
			maxz = aabbs[i].topCorner[2];
	}
	this->setNodeBottom(node,Vec4f(minx,miny,minz,-1));
	this->setNodeTop(node,Vec4f(maxx,maxy,maxz,-1));

	vector<MetaPrimitive*> empty;
	buildNodeNoOverlap(0,aabbs,empty);
}

void CompactBvh::buildNode(int node,const vector<AABBLite>& primitives, const vector<MetaPrimitive*>& splitPrimitives){
	//If stoping criteria not reached
	if(  node < maxNbOfNonLeafNodes && primitives.size()>1){ //this->aabb.topCorner[0] - this->aabb.bottomCorner[0] > 0.1 &&
	
		//Decide in which direction to split
		Vec4f bottom = nodeBottom(node);
		Vec4f top = nodeTop(node);
		float xSise = (top[0]-bottom[0]);
		float ySise = (top[1]-bottom[1]);
		float zSise = (top[2]-bottom[2]);

		int axis = 0;
		if( xSise < ySise){
			if(ySise < zSise){
				axis = 2;//splitz 
			}else{
				axis = 1;//splity
			}
		}else if(xSise < zSise){
			axis = 2;//splitz
		}else{
			axis = 0;//splitx
		}

		MinLessCompL mincomp;
		mincomp.axis = axis;
		MaxLessCompL maxcomp;
		maxcomp.axis = axis;

		vector<AABBLite> sortedAABBS;
		sortedAABBS.insert(sortedAABBS.begin(),primitives.begin(),primitives.end());
		//sort the aabbs on the x-axis according to the low corner
		std::sort(sortedAABBS.begin(),sortedAABBS.end(),mincomp);
		int mid = primitives.size()/2; 

		/********************* Construct child 1 AABB *********************/
		vector<AABBLite> prims1;
		prims1.insert(prims1.begin(),sortedAABBS.begin(),sortedAABBS.begin()+mid);
		float minx = BIG, miny = BIG, minz = BIG, maxx = -BIG, maxy = -BIG, maxz = -BIG;
		for(unsigned int i  = 0; i < prims1.size(); i++){
			if(prims1[i].bottomCorner[0] < minx)
				minx = prims1[i].bottomCorner[0];
			if(prims1[i].topCorner[0] > maxx)
				maxx = prims1[i].topCorner[0];
			if(prims1[i].bottomCorner[1] < miny)
				miny = prims1[i].bottomCorner[1];
			if(prims1[i].topCorner[1] > maxy)
				maxy = prims1[i].topCorner[1];
			if(prims1[i].bottomCorner[2] < minz)
				minz = prims1[i].bottomCorner[2];
			if(prims1[i].topCorner[2] > maxz)
				maxz = prims1[i].topCorner[2];
		}
		this->setNodeBottom(child1(node),Vec4f(minx,miny,minz,-1));
		this->setNodeTop(child1(node),Vec4f(maxx,maxy,maxz,-1));
		/******************************************/
		
		/********************* Construct child 1 AABB *********************/
		vector<AABBLite> prims2;
		prims2.insert(prims2.begin(),sortedAABBS.begin()+mid,sortedAABBS.end());
		minx = BIG; miny = BIG; minz = BIG; maxx = -BIG; maxy = -BIG; maxz = -BIG;
		for(unsigned int i  = 0; i < prims2.size(); i++){
			if(prims2[i].bottomCorner[0] < minx)
				minx = prims2[i].bottomCorner[0];
			if(prims2[i].topCorner[0] > maxx)
				maxx = prims2[i].topCorner[0];
			if(prims2[i].bottomCorner[1] < miny)
				miny = prims2[i].bottomCorner[1];
			if(prims2[i].topCorner[1] > maxy)
				maxy = prims2[i].topCorner[1];
			if(prims2[i].bottomCorner[2] < minz)
				minz = prims2[i].bottomCorner[2];
			if(prims2[i].topCorner[2] > maxz)
				maxz = prims2[i].topCorner[2];
		}
		this->setNodeBottom(child2(node),Vec4f(minx,miny,minz,-1));
		this->setNodeTop(child2(node),Vec4f(maxx,maxy,maxz,-1));
		/******************************************/
	
		/*********************** DISTRIBUTE SPLIT TO CHILD 1 *****************/
		vector<MetaPrimitive*> split1;
		for( int i = 0;  i < prims2.size(); i++){
			if(Intersects(prims2[i].primitive,child2(node)))//
				split1.push_back(prims2[i].primitive);
		}
		for( int i = 0;  i < splitPrimitives.size(); i++){
			if(Intersects(splitPrimitives[i],child1(node)))//
				split1.push_back(splitPrimitives[i]);
		}
		/*********************************************************************/

		/*********************** DISTRIBUTE SPLIT TO CHILD 2 *****************/
		vector<MetaPrimitive*> split2;
		for( int i = 0; i < prims1.size(); i++){
			if(Intersects(prims1[i].primitive,child2(node)))//
				split2.push_back(prims1[i].primitive);
		}
		for( int i = 0; i < splitPrimitives.size(); i++){
			if(Intersects(splitPrimitives[i],child2(node)))//
				split2.push_back(splitPrimitives[i]);
		}
		/*********************************************************************/

		this->buildNode(child1(node),prims1,split1);
		this->buildNode(child2(node),prims2,split2);

	}else{
		Vec4f spheres(-1);
		Vec4f tubes(-1);

		MetaPrimitive* prim = primitives[0].primitive;

		int nbOfSph = 0;
		int nbOfTb = 0;

		for(int i = 0; i < 4 && i < primitives.size() ; i++){
			if(prim->getType() == "MetaTube"){
				MetaTube* tube = static_cast<MetaTube*>(primitives[i].primitive);
				tubes[i] = tube->id; 
				nbOfTb++;
			}else if(prim->getType() == "MetaBall"){
				Metaball* ball = static_cast<Metaball*>(primitives[i].primitive);
				spheres[i] = ball->id;
				nbOfSph++;
			}
		}
		for(int i = 0; i < 4 && i < splitPrimitives.size() ; i++){
			prim = splitPrimitives[i];
			if(prim->getType() == "MetaTube"){
				if(nbOfTb < 4){
					MetaTube* tube = static_cast<MetaTube*>(prim);
					tubes[nbOfTb] = tube->id;
					nbOfTb++;
				}
			}else if(prim->getType() == "MetaBall"){
				if(nbOfSph < 4){
					Metaball* ball = static_cast<Metaball*>(prim);
					spheres[nbOfSph] = ball->id;
					nbOfSph++;
				}
			}
		}

		int primsLocation = nbOfLeafs + maxNbOfNodes;
		setTags(node,primsLocation);

		setNodeBottom(primsLocation,spheres);
		setNodeTop(primsLocation,tubes);

		nbOfLeafs++;
	}

}

void CompactBvh::buildNodeNoOverlap(int node,const vector<AABBLite>& primitives, const vector<MetaPrimitive*>& splitPrimitives){
	//If stoping criteria not reached
	if(  node < maxNbOfNonLeafNodes && primitives.size()>1){ //this->aabb.topCorner[0] - this->aabb.bottomCorner[0] > 0.1 &&
	
		//Decide in which direction to split
		Vec4f bottom = nodeBottom(node);
		Vec4f top = nodeTop(node);
		float xSise = (top[0]-bottom[0]);
		float ySise = (top[1]-bottom[1]);
		float zSise = (top[2]-bottom[2]);

		int axis = 0;
		if( xSise < ySise){
			if(ySise < zSise){
				axis = 2;//splitz 
			}else{
				axis = 1;//splity
			}
		}else if(xSise < zSise){
			axis = 2;//splitz
		}else{
			axis = 0;//splitx
		}

		MinLessCompL mincomp;
		mincomp.axis = axis;
		MaxLessCompL maxcomp;
		maxcomp.axis = axis;

		vector<AABBLite> sortedAABBS;
		sortedAABBS.insert(sortedAABBS.begin(),primitives.begin(),primitives.end());
		//sort the aabbs on the x-axis according to the low corner
		std::sort(sortedAABBS.begin(),sortedAABBS.end(),mincomp);
		float mid = 0.5f*(bottom[axis]+top[axis]); 


		/********************* Distribute to childs *********************/
		vector<AABBLite> prims1;
		vector<AABBLite> prims2;
		for(int i = 0; i < sortedAABBS.size(); i++){
			float c = 0.5f*(sortedAABBS[i].topCorner[axis]+sortedAABBS[i].bottomCorner[axis]);
			if( c <= mid)
				prims1.push_back(sortedAABBS[i]);
			else if ( c > mid)
				prims2.push_back(sortedAABBS[i]);
		}
		
		//Adjust spliting position
		for(int i = 0; i < prims1.size(); i++){
			if(prims1[i].topCorner[axis] > mid)
				mid = prims1[i].topCorner[axis];
		}
		AABBLite aabb1;
		aabb1.bottomCorner = Vec3f(bottom);
		aabb1.topCorner = Vec3f(top);
		aabb1.topCorner[axis] = mid;
		AABBLite aabb2;
		aabb2.bottomCorner = Vec3f(bottom);
		aabb2.topCorner = Vec3f(top);
		aabb2.bottomCorner[axis] = mid;

		/********************* Redistribute to childs *********************/
		prims1.clear();
		prims2.clear();
		for(int i = 0; i < sortedAABBS.size(); i++){
			float c = 0.5f*(sortedAABBS[i].topCorner[axis]+sortedAABBS[i].bottomCorner[axis]);
			if( c <= mid)
				prims1.push_back(sortedAABBS[i]);
			else if ( c > mid)
				prims2.push_back(sortedAABBS[i]);
		}



		/*********************** DISTRIBUTE SPLIT TO CHILD 1 *****************/
		vector<MetaPrimitive*> split1;
		for( int i = 0;  i < prims2.size(); i++){
			if(Intersects(prims2[i].primitive,aabb1))//
				split1.push_back(prims2[i].primitive);
		}
		for( int i = 0;  i < splitPrimitives.size(); i++){
			if(Intersects(splitPrimitives[i],aabb1))//
				split1.push_back(splitPrimitives[i]);
		}
		/*********************************************************************/

		/*********************** DISTRIBUTE SPLIT TO CHILD 2 *****************/
		vector<MetaPrimitive*> split2;
		for( int i = 0; i < prims1.size(); i++){
			if(Intersects(prims1[i].primitive,aabb2))//
				split2.push_back(prims1[i].primitive);
		}
		for( int i = 0; i < splitPrimitives.size(); i++){
			if(Intersects(splitPrimitives[i],aabb2))//
				split2.push_back(splitPrimitives[i]);
		}
		/************** COMPUTE AABB of the prims and splits  aabbs ************/
		AABBLite primAABB1;
		primAABB1.bottomCorner = Vec3f(BIG);
		primAABB1.topCorner = Vec3f(-BIG);
		for(int i = 0; i < prims1.size(); i++){
			Union(primAABB1,Restriction(prims1[i].primitive,aabb1));
			//Union(primAABB1,prims1[i]);
		}
		for(int i = 0; i < split1.size(); i++){
			Union(primAABB1,Restriction(split1[i],aabb1));
			//Union(primAABB1,split1[i]->aabb);
		}

		AABBLite primAABB2;
		primAABB2.bottomCorner = Vec3f(BIG);
		primAABB2.topCorner = Vec3f(-BIG);
		for(int i = 0; i < prims2.size(); i++){
			Union(primAABB2,Restriction(prims2[i].primitive,aabb2));
			//Union(primAABB2,prims2[i]);
		}
		for(int i = 0; i < split2.size(); i++){
			Union(primAABB2,Restriction(split2[i],aabb2));
			//Union(primAABB2,split2[i]->aabb);
		}


		/******** INTERSECT AABBS and PRIMS AABBs ************/
		Intersection(aabb1,primAABB1);
		Intersection(aabb2,primAABB2);
		
		
		this->setNodeBottom(child1(node),Vec4f(aabb1.bottomCorner[0],aabb1.bottomCorner[1],aabb1.bottomCorner[2],-1));
		this->setNodeTop(child1(node),Vec4f(aabb1.topCorner[0],aabb1.topCorner[1],aabb1.topCorner[2],-1));
	

		this->setNodeBottom(child2(node),Vec4f(aabb2.bottomCorner[0],aabb2.bottomCorner[1],aabb2.bottomCorner[2],-1));
		this->setNodeTop(child2(node),Vec4f(aabb2.topCorner[0],aabb2.topCorner[1],aabb2.topCorner[2],-1));
		/******************************************/
	


		this->buildNodeNoOverlap(child1(node),prims1,split1);
		this->buildNodeNoOverlap(child2(node),prims2,split2);

	}else{
		Vec4f spheres(-1);
		Vec4f tubes(-1);

		

		int nbOfSph = 0;
		int nbOfTb = 0;
		
		for(int i = 0; i < 4 && i < primitives.size() ; i++){
			if(primitives[i].primitive->getType() == "MetaTube"){
				MetaTube* tube = static_cast<MetaTube*>(primitives[i].primitive);
				tubes[i] = tube->id; 
				nbOfTb++;
			}else if(primitives[i].primitive->getType() == "MetaBall"){
				Metaball* ball = static_cast<Metaball*>(primitives[i].primitive);
				spheres[i] = ball->id;
				nbOfSph++;
			}
		}
		
		for(int i = 0; i < 4 && i < splitPrimitives.size() ; i++){
			MetaPrimitive* prim = splitPrimitives[i];
			if(prim->getType() == "MetaTube"){
				if(nbOfTb < 4){
					MetaTube* tube = static_cast<MetaTube*>(prim);
					tubes[nbOfTb] = tube->id;
					nbOfTb++;
				}
			}else if(prim->getType() == "MetaBall"){
				if(nbOfSph < 4){
					Metaball* ball = static_cast<Metaball*>(prim);
					spheres[nbOfSph] = ball->id;
					nbOfSph++;
				}
			}
		}

		int primsLocation = nbOfLeafs + maxNbOfNodes;
		setTags(node,primsLocation);

		setNodeBottom(primsLocation,spheres);
		setNodeTop(primsLocation,tubes);

		nbOfLeafs++;
	}

}

void CompactBvh::render(int level)const{


	for(int lvl = 0; lvl <= level; lvl++ ){
		int firstAtLevel = powf(2,lvl)-1;
		int firstAtNextLevel = powf(2,lvl+1)-1;
		for(int node = firstAtLevel; node < firstAtNextLevel; node++){
			if(nodeBottom(node)[3] != -1 || lvl == level)
				drawBox(Vec3f(nodeBottom(node)),Vec3f(nodeTop(node)));
		}
	}
}

void CompactBvh::print()const{
	for(int i = 0; i < this->maxNbOfNodes; i++){
		string isLeaf = nodeBottom(i)[3]!=-1?"leaf ":"";
		cout<<"node "<<isLeaf<<i<<" "<<nodeBottom(i)<<nodeTop(i)<<endl;
	}
	for(int i = 0; i < this->nbOfLeafs; i++){
		cout<<"prim of leaf "<<i<<nodeBottom(i+this->maxNbOfNodes)<<nodeTop(i+this->maxNbOfNodes)<<endl;
	}
}

/*
void resizeRight(int axis, float mid, AABB& ab){
	Vec3f boxMiddle = 0.5f*(ab.topCorner+ab.bottomCorner);
	ab.bottomCorner[axis] = mid;
	//Get the primitive info and clip the segment against the aabb
	MetaTube* tube = static_cast<MetaTube*>((*(ab.primitives.begin())));
	//MetaTube normal

	Vec3f sortP1;
	Vec3f sortP2;
	if(tube->worldP1[axis] <= tube->worldP2[axis]){
		sortP1 = tube->worldP1;
		sortP2 = tube->worldP2;
	}else{
		sortP1 = tube->worldP2;
		sortP2 = tube->worldP1;
	}

	Vec3f P1;
	Vec3f P2;
	Vec3f axisV(axis==0?1:0,axis==1?1:0,axis==2?1:0);
	Vec3f tn = normalize(tube->worldP2-tube->worldP1);

	// Project the segment points on the faces of the box
	if( ab.bottomCorner[axis] < sortP1[axis] ){//ortogonal
		P1 = sortP1;
		P1[axis] = ab.bottomCorner[axis];
	}else if( ab.bottomCorner[axis] < sortP2[axis]){//on segment
		P1 = sortP1 + (dot(axisV,ab.bottomCorner-sortP1)/dot(axisV,tn))*tn;
	}else{//orthogonal
		P1 = sortP2;
		P1[axis] = ab.bottomCorner[axis];
	}
	if( ab.topCorner[axis] < sortP1[axis] ){//ortogonal
		P2 = sortP1;
		P2[axis] = ab.topCorner[axis];
	}else if( ab.topCorner[axis] < sortP2[axis]){//on segment
		P2 = sortP2 + (dot(axisV,ab.topCorner-sortP2)/dot(axisV,tn))*tn;
	}else{//orthogonal
		P2 = sortP2;
		P2[axis] = ab.topCorner[axis];
	}
	
	//Section of the MetaTube with the plane at I
	//projected 
	float dntn = tn[axis];
	Vec3f tn0 = tn - dot(tn,Vec3f(0,1,0))*Vec3f(0,1,0);
	Vec3f tn1 = tn - dot(tn,Vec3f(1,0,0))*Vec3f(0,1,0);
	//Section great radiuses
	float sx = abs(tube->r1 / dntn);
	float sy = abs(tube->r1 / dntn);

	Vec3f I13D; 
	if(mid < sortP1[axis]){
		I13D = sortP1;
		I13D[axis] = mid;
		sx = sy = sqrtf(sqr<float>(tube->r1)-sqr<float>(mid - sortP1[axis]));
	}else if(mid < sortP2[axis]){
		I13D = boxMiddle;
		sx = tube->r1;//abs(MetaTube->r1 / dntn);//wrong
		sy = tube->r1;//abs(MetaTube->r1 / dntn);//wrond
	}else{
		I13D = sortP2;
		I13D[axis] = mid;
		sx = sy = sqrtf(sqr<float>(tube->r2)-sqr<float>(mid - sortP2[axis]));
	}

	//centroid of the section
	Vec2f I1 = v3DTov2D2(I13D,axis);
	//Bounds of the section
	Vec2f sBoundsMin;
	Vec2f sBoundsMax;
	sBoundsMin = I1 - Vec2f(sx,sy);//Vec2f(sx,sy);
	sBoundsMax = I1 + Vec2f(sx,sy);//Vec2f(sx,sy);


	//Clib ab2
	//Projection of P2 on the plane
	Vec2f p2 = v3DTov2D2(P2,axis);
	//Bounds of the projection of p2
	Vec2f p2BoundsMin;
	Vec2f p2BoundsMax;
	if( P2[axis] < sortP1[axis]){ //Projection of the sphere
		sx = sy = sqrtf(sqr<float>(tube->r1)-sqr<float>(P2[axis] - sortP1[axis]));
	}else if(P2[axis] < sortP2[axis]){ //Section of the intersection
		sx = tube->r1;//abs(MetaTube->r1 / dntn);//wrong
		sy = tube->r1;//abs(MetaTube->r1 / dntn);//wrond
	}else if(P2[axis] < sortP2[axis] + tube->r2){
		sx = sy = sqrtf(sqr<float>(tube->r2)-sqr<float>(P2[axis] - sortP2[axis]));
	}else{
		sx = tube->r1;//abs(MetaTube->r1 / dntn);//wrong
		sy = tube->r1;//abs(MetaTube->r1 / dntn);//wrond
	}
	p2BoundsMin = p2 - Vec2f(sx,sy);//Vec2f(sx,sy);
	p2BoundsMax = p2 + Vec2f(sx,sy);//Vec2f(sx,sy);
	//Bounds of the two bounds
	float min0 = min(p2BoundsMin[0],sBoundsMin[0]);
	float min1 = min(p2BoundsMin[1],sBoundsMin[1]);
	float max0 = max(p2BoundsMax[0],sBoundsMax[0]);
	float max1 = max(p2BoundsMax[1],sBoundsMax[1]);

	//intersect with old aabb
	Vec2f ProjBottom = v3DTov2D2(ab.bottomCorner,axis);
	Vec2f ProjTop = v3DTov2D2(ab.topCorner,axis);
	min0 = max(min0, ProjBottom[0]);
	min1 = max(min1, ProjBottom[1]);
	max0 = min(max0, ProjTop[0]);
	max1 = min(max1, ProjTop[1]);


	switch(axis){ //Layout : splix (y,z), spliy (x,z), spliz (x,y)
		case 0:
			ab.bottomCorner[1] = min0;
			ab.bottomCorner[2] = min1;
			ab.topCorner[1] = max0;
			ab.topCorner[2] = max1;
			break;
		case 1:
			ab.bottomCorner[0] = min0;
			ab.bottomCorner[2] = min1;
			ab.topCorner[0] = max0;
			ab.topCorner[2] = max1;
			break;
		case 2:
			ab.bottomCorner[0] = min0;
			ab.bottomCorner[1] = min1;
			ab.topCorner[0] = max0;
			ab.topCorner[1] = max1;
			break;
	}
}
*/

/*
void resizeLeft(int axis, float mid, AABB& ab){
	Vec3f boxMiddle = 0.5f*(ab.topCorner+ab.bottomCorner);
	ab.topCorner[axis] = mid;
	//Get the primitive info and clip the segment against the aabb
	MetaTube* tube = static_cast<MetaTube*>((*(ab.primitives.begin())));
	//MetaTube normal

	Vec3f sortP1;
	Vec3f sortP2;
	if(tube->worldP1[axis] <= tube->worldP2[axis]){
		sortP1 = tube->worldP1;
		sortP2 = tube->worldP2;
	}else{
		sortP1 = tube->worldP2;
		sortP2 = tube->worldP1;
	}

	Vec3f P1;
	Vec3f P2;
	Vec3f axisV(axis==0?1:0,axis==1?1:0,axis==2?1:0);
	Vec3f tn = normalize(tube->worldP2-tube->worldP1);

	// Project the segment points on the faces of the box
	if( ab.bottomCorner[axis] < sortP1[axis] ){//ortogonal
		P1 = sortP1;
		P1[axis] = ab.bottomCorner[axis];
	}else if( ab.bottomCorner[axis] < sortP2[axis]){//on segment
		P1 = sortP1 + (dot(axisV,ab.bottomCorner-sortP1)/dot(axisV,tn))*tn;
	}else{//orthogonal
		P1 = sortP2;
		P1[axis] = ab.bottomCorner[axis];
	}
	if( ab.topCorner[axis] < sortP1[axis] ){//ortogonal
		P2 = sortP1;
		P2[axis] = ab.topCorner[axis];
	}else if( ab.topCorner[axis] < sortP2[axis]){//on segment
		P2 = sortP2 + (dot(axisV,ab.topCorner-sortP2)/dot(axisV,tn))*tn;
	}else{//orthogonal
		P2 = sortP2;
		P2[axis] = ab.topCorner[axis];
	}
	
	//Section of the MetaTube with the plane at I
	//projected 
	float dntn = tn[axis];
	Vec3f tn0 = tn - dot(tn,Vec3f(0,1,0))*Vec3f(0,1,0);
	Vec3f tn1 = tn - dot(tn,Vec3f(1,0,0))*Vec3f(0,1,0);
	//Section great radiuses
	float sx = abs(tube->r1 / dntn);
	float sy = abs(tube->r1 / dntn);

	Vec3f I13D; 
	if(mid < sortP1[axis]){
		I13D = sortP1;
		I13D[axis] = mid;
		sx = sy = sqrtf(sqr<float>(tube->r1)-sqr<float>(mid - sortP1[axis]));
	}else if(mid < sortP2[axis]){
		I13D = boxMiddle;
		sx = tube->r1;//abs(MetaTube->r1 / dntn);//wrong
		sy = tube->r1;//abs(MetaTube->r1 / dntn);//wrond
	}else{
		I13D = sortP2;
		I13D[axis] = mid;
		sx = sy = sqrtf(sqr<float>(tube->r2)-sqr<float>(mid - sortP2[axis]));
	}

	//centroid of the section
	Vec2f I1 = v3DTov2D2(I13D,axis);
	//Bounds of the section
	Vec2f sBoundsMin;
	Vec2f sBoundsMax;
	sBoundsMin = I1 - Vec2f(sx,sy);//Vec2f(sx,sy);
	sBoundsMax = I1 + Vec2f(sx,sy);//Vec2f(sx,sy);

	//Clib ab1
	//Projection of P1 on the plane if P1 is in the box
	Vec2f p1 = v3DTov2D2(P1,axis);
	//Bounds of the projection of p1
	Vec2f p1BoundsMin;
	Vec2f p1BoundsMax;

	if( P1[axis] > sortP2[axis]){
		sx = sy = sqrtf(sqr<float>(tube->r2)-sqr<float>(P1[axis] - sortP2[axis]));
	}else if( P1[axis] > sortP1[axis]){
		sx = tube->r1;//abs(MetaTube->r1 / dntn);//wrong
		sy = tube->r1;//abs(MetaTube->r1 / dntn);//wrond
	}else if( P1[axis] > sortP1[axis]-tube->r1+0.000001){
		sx = sy = sqrtf(sqr<float>(tube->r1)-sqr<float>(P1[axis] - sortP1[axis]));
	}else{
		sx = tube->r1;
		sy = tube->r1;
	}*/
/*
	if( P1[axis] <= sortP1[axis] - MetaTube->r1){
		sx = MetaTube->r1;
		sy = MetaTube->r1;
	}else if( P1[axis] < sortP1[axis]){ //Projection of the sphere
		sx = sy = sqrtf(sqr<float>(MetaTube->r1)-sqr<float>(P1[axis] - sortP1[axis]));
	}else if(P1[axis] < sortP2[axis]){ //Section of the intersection
		sx = MetaTube->r1;//abs(MetaTube->r1 / dntn);//wrong
		sy = MetaTube->r1;//abs(MetaTube->r1 / dntn);//wrond
	}else{
		sx = sy = sqrtf(sqr<float>(MetaTube->r2)-sqr<float>(P1[axis] - sortP2[axis]));
	}
	*//*
	p1BoundsMin = p1 - Vec2f(sx,sy);//Vec2f(sx,sy);
	p1BoundsMax = p1 + Vec2f(sx,sy);//Vec2f(sx,sy);

	//Bounds of the two bounds
	float min0 = min(p1BoundsMin[0],sBoundsMin[0]);
	float min1 = min(p1BoundsMin[1],sBoundsMin[1]);
	float max0 = max(p1BoundsMax[0],sBoundsMax[0]);
	float max1 = max(p1BoundsMax[1],sBoundsMax[1]);

	//intersect with old aabb
	Vec2f ProjBottom = v3DTov2D2(ab.bottomCorner,axis);
	Vec2f ProjTop = v3DTov2D2(ab.topCorner,axis);
	min0 = max(min0, ProjBottom[0]);
	min1 = max(min1, ProjBottom[1]);
	max0 = min(max0, ProjTop[0]);
	max1 = min(max1, ProjTop[1]);

	switch(axis){ //Layout : splix (y,z), spliy (x,z), spliz (x,y)
	case 0:
		ab.bottomCorner[1] = min0;
		ab.bottomCorner[2] = min1;
		ab.topCorner[1] = max0;
		ab.topCorner[2] = max1;
		break;
	case 1:
		ab.bottomCorner[0] = min0;
		ab.bottomCorner[2] = min1;
		ab.topCorner[0] = max0;
		ab.topCorner[2] = max1;
		break;
	case 2:
		ab.bottomCorner[0] = min0;
		ab.bottomCorner[1] = min1;
		ab.topCorner[0] = max0;
		ab.topCorner[1] = max1;
		break;
	}
}*/

/*			
			//sort the aabbs on the x-axis according to the low corner
			std::sort(aabbs.begin(),aabbs.end(),mincomp);

			//Isolate the ones completely on the left side
			float midx = 0.5f*(this->aabb.topCorner[axis]+this->aabb.bottomCorner[axis]);
			vector<AABB>::iterator mid = aabbs.begin();
			while(mid!=aabbs.end()&&(mid->bottomCorner[axis]<midx))
				++mid;

			aabb2s.insert(aabb2s.begin(),mid,aabbs.end());
			aabbs.erase(mid,aabbs.end());

			//Isolate the one completely on the right side
			std::sort(aabbs.begin(),aabbs.end(),maxcomp);
			vector<AABB>::reverse_iterator midr = aabbs.rbegin();
			int i = aabbs.size()-1;
			while(i>0&&(aabbs[i].topCorner[axis]>midx)){
				--midr;
				i--;
			}

			vector<AABB>::iterator mi = aabbs.begin()+i;
			aabb1s.insert(aabb1s.begin(),aabbs.begin(),mi);
			aabbs.erase(aabbs.begin(),mi);

			aabb1s.insert(aabb1s.begin(),aabbs.begin(),aabbs.end());
			aabb2s.insert(aabb2s.begin(),aabbs.begin(),aabbs.end());

			//Initialize the childs
			this->child1->initialize(aabb1s);
			this->child2->initialize(aabb2s);*/