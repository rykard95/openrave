// Copyright (C) 2006-2009 Carnegie Mellon University (rdiankov@cs.cmu.edu)
//
// This file is part of OpenRAVE.
// OpenRAVE is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
#ifndef RAVE_PLANNERS_H
#define RAVE_PLANNERS_H

#include "plugindefs.h"

// interval types   ( , )      ( , ]       [ , )      [ , ]
enum IntervalType { OPEN=0,  OPEN_START,  OPEN_END,  CLOSED };

enum ExtendType {
    ET_Failed=0,
    ET_Sucess=1,
    ET_Connected=2
};

// sorted by increasing getvalue
template <class T, class S>
class BinarySearchTree
{
public:
    BinarySearchTree() { Reset(); }

    // other global definitions
    void Add(T& pex)
    {
        assert( pex != NULL );
        
        switch(blocks.size()) {
		    case 0:
                blocks.push_back(pex);
                return;
		    case 1:
                if( blocks.front()->getvalue() < pex->getvalue() ) {
                    blocks.push_back(pex);
                }
                else blocks.insert(blocks.begin(), pex);
                
                return;
                
		    default: {
                int imin = 0, imax = (int)blocks.size(), imid;
                
                while(imin < imax) {
                    imid = (imin+imax)>>1;
                    
                    if( blocks[imid]->getvalue() > pex->getvalue() ) imax = imid;
                    else imin = imid+1;
                }
                
                blocks.insert(blocks.begin()+imin, pex);
                return;
            }
        }
    }
    
    ///< returns the index into blocks
    int Get(S& s)
    {
        switch(blocks.size()) {
		    case 1: return 0;
		    case 2: return blocks.front()->getvalue() < s;    
		    default: {
                int imin = 0, imax = blocks.size()-1, imid;
                
                while(imin < imax) {
                    imid = (imin+imax)>>1;
                    
                    if( blocks[imid]->getvalue() > s ) imax = imid;
                    else if( blocks[imid]->getvalue() == s ) return imid;
                    else imin = imid+1;
                }
                
                return imin;
            }
        }
    }

    void Reset()
    {
        blocks.clear();
        blocks.reserve(1<<16);
    }
    
	vector<T> blocks;
};

class SimpleDistMetric
{
 public:
 SimpleDistMetric(RobotBasePtr robot) : _robot(robot)
    {
        float ftransweight = 2;
        weights.resize(0);
        vector<int>::const_iterator it;
        FORIT(it, _robot->GetActiveJointIndices()) weights.push_back(_robot->GetJointWeight(*it));
        if( _robot->GetAffineDOF() & RobotBase::DOF_X ) weights.push_back(ftransweight);
        if( _robot->GetAffineDOF() & RobotBase::DOF_Y ) weights.push_back(ftransweight);
        if( _robot->GetAffineDOF() & RobotBase::DOF_Z ) weights.push_back(ftransweight);
        if( _robot->GetAffineDOF() & RobotBase::DOF_RotationAxis ) weights.push_back(ftransweight);
        else if( _robot->GetAffineDOF() & RobotBase::DOF_RotationQuat ) {
            weights.push_back(0.4f);
            weights.push_back(0.4f);
            weights.push_back(0.4f);
            weights.push_back(0.4f);
        }
    }

    virtual dReal Eval(const std::vector<dReal>& c0, const std::vector<dReal>& c1)
    {
        dReal out = 0;
        for(int i=0; i < _robot->GetActiveDOF(); i++)
            out += weights[i] * (c0[i]-c1[i])*(c0[i]-c1[i]);
            
        return RaveSqrt(out);
    }

 protected:
    RobotBasePtr _robot;
    vector<dReal> weights;
};

class SimpleCostMetric
{
 public:
    SimpleCostMetric(RobotBasePtr robot) {}
    virtual float Eval(const vector<dReal>& pConfiguration) { return 1; }
};

class SimpleGoalMetric
{
public:       
 SimpleGoalMetric(RobotBasePtr robot, dReal thresh=0.01f) : _robot(robot), _thresh(thresh) {}
    
    //checks if pConf is within this cone (note: only works in 3D)
    dReal Eval(const vector<dReal>& c1)
    {
        _robot->SetActiveDOFValues(c1);
        Transform cur = _robot->GetActiveManipulator()->GetEndEffectorTransform();
        dReal f = RaveSqrt(lengthsqr3(tgoal.trans - cur.trans));
        return f < _thresh ? 0 : f;
    }

    Transform tgoal; // workspace goal

 private:
    RobotBasePtr _robot;
    dReal _thresh;
};


class SimpleSampleFunction
{
 public:
 SimpleSampleFunction(RobotBasePtr robot, const boost::function<dReal(const std::vector<dReal>&, const std::vector<dReal>&)>& distmetricfn) : _robot(robot), _distmetricfn(distmetricfn) {
        _robot->GetActiveDOFLimits(lower, upper);
        range.resize(lower.size());
        for(int i = 0; i < (int)range.size(); ++i)
            range[i] = upper[i] - lower[i];
    }
    virtual bool Sample(vector<dReal>& pNewSample) {
        pNewSample.resize(lower.size());
        for (size_t i = 0; i < lower.size(); i++)
            pNewSample[i] = lower[i] + RaveRandomFloat()*range[i];
        return true;
    }

    virtual bool SampleNeigh(vector<dReal>& pNewSample, const vector<dReal>& pCurSample, dReal fRadius)
    {
        BOOST_ASSERT(pCurSample.size()==lower.size());
        pNewSample.resize(lower.size());
        int dof = lower.size();
        for (int i = 0; i < dof; i++)
            pNewSample[i] = pCurSample[i] + 10.0f*fRadius*(RaveRandomFloat()-0.5f);

        // normalize
        dReal fRatio = fRatio*RaveRandomFloat();
            
        //assert(_robot->ConfigDist(&_vzero[0], &_vSampleConfig[0]) < B+1);
        while(_distmetricfn(pNewSample,pCurSample) > fRatio ) {
            for (int i = 0; i < dof; i++)
                pNewSample[i] = 0.5f*pCurSample[i]+0.5f*pNewSample[i];
        }
            
        while(_distmetricfn(pNewSample, pCurSample) < fRatio ) {
            for (int i = 0; i < dof; i++)
                pNewSample[i] = 1.2f*pNewSample[i]-0.2f*pCurSample[i];
        }

        for (int i = 0; i < dof; i++) {
            if( pNewSample[i] < lower[i] )
                pNewSample[i] = lower[i];
            else if( pNewSample[i] > upper[i] )
                pNewSample[i] = upper[i];
        }

        return true;
    }

 protected:
    RobotBasePtr _robot;
    vector<dReal> lower, upper, range;
    boost::function<dReal(const std::vector<dReal>&, const std::vector<dReal>&)> _distmetricfn;
};

struct SimpleNode
{
SimpleNode(int parent, const vector<dReal>& q) : parent(parent), q(q) {}
    int parent;
    vector<dReal> q; // the configuration immediately follows the struct
};

class SpatialTreeBase
{
 public:
    virtual int AddNode(int parent, const vector<dReal>& config) = 0;
    virtual int GetNN(const vector<dReal>& q) = 0;
    virtual const vector<dReal>& GetConfig(int inode) = 0;
    virtual ExtendType Extend(const vector<dReal>& pTargetConfig, int& lastindex, bool bOneStep=false) = 0;
    virtual int GetDOF() = 0;
};

template <typename Planner, typename Node>
class SpatialTree : public SpatialTreeBase
{
 public:
    SpatialTree() {
        _fStepLength = 0.04f;
        _dof = 0;
        _fBestDist = 0;
        _nodes.reserve(5000);
    }

    ~SpatialTree(){}
    
    virtual void Reset(Planner planner, int dof=0)
    {
        _planner = planner;

        typename vector<Node*>::iterator it;
        FORIT(it, _nodes)
            delete *it;
        _nodes.resize(0);

        if( dof > 0 ) {
            _vNewConfig.resize(dof);
            _dof = dof;
        }   
    }

    virtual int AddNode(int parent, const vector<dReal>& config)
    {
        _nodes.push_back(new Node(parent,config));
        return (int)_nodes.size()-1;
    }

    ///< return the nearest neighbor
    virtual int GetNN(const vector<dReal>& q)
    {
        if( _nodes.size() == 0 )
            return -1;

        int ibest = -1;
        float fbest = 0;
        FOREACH(itnode, _nodes) {
            float f = _distmetricfn(q, (*itnode)->q);
            if( ibest < 0 || f < fbest ) {
                ibest = (int)(itnode-_nodes.begin());
                fbest = f;
            }
        }

        _fBestDist = fbest;
        return ibest;
    }

    /// extends toward pNewConfig
    /// \return true if extension reached pNewConfig
    virtual ExtendType Extend(const vector<dReal>& pTargetConfig, int& lastindex, bool bOneStep=false)
    {
        // get the nearest neighbor
        lastindex = GetNN(pTargetConfig);
        Node* pnode = _nodes[lastindex];
        bool bHasAdded = false;

        // extend
        while(1) {
            float fdist = _distmetricfn(pnode->q, pTargetConfig);

            if( fdist > _fStepLength ) fdist = _fStepLength / fdist;
            else {
                return ET_Connected;
            }
        
            for(int i = 0; i < _dof; ++i)
                _vNewConfig[i] = pnode->q[i] + (pTargetConfig[i]-pnode->q[i])*fdist;
        
            // project to constraints
            if( !!_planner->GetParameters()._constraintfn ) {
                if( !_planner->GetParameters()._constraintfn(pnode->q, _vNewConfig, 0) ) {
                    if(bHasAdded)
                        return ET_Sucess;
                    return ET_Failed;
                }
            }

            if( _planner->_CheckCollision(pnode->q, _vNewConfig, OPEN_START) ) {
                if(bHasAdded)
                    return ET_Sucess;
                return ET_Failed;
            }

            lastindex = AddNode(lastindex, _vNewConfig);
            pnode = _nodes[lastindex];
            bHasAdded = true;
            if( bOneStep )
                return ET_Connected;
        }
    
        return ET_Failed;
    }

    virtual const vector<dReal>& GetConfig(int inode) { return _nodes.at(inode)->q; }
    virtual int GetDOF() { return _dof; }

    vector<Node*> _nodes;
    boost::function<dReal(const std::vector<dReal>&, const std::vector<dReal>&)> _distmetricfn;

    dReal _fBestDist; ///< valid after a call to GetNN
    dReal _fStepLength;

 private:
    vector<dReal> _vNewConfig;
    Planner _planner;
    int _dof;
};

class ExplorationParameters : public PlannerBase::PlannerParameters
{
 public:
 ExplorationParameters() : _fExploreProb(0), _nExpectedDataSize(100) {}
        
    dReal _fExploreProb;
    int _nExpectedDataSize;
        
 protected:
    // save the extra data to XML
    virtual bool serialize(std::ostream& O) const
    {
        if( !PlannerParameters::serialize(O) )
            return false;
        O << "<exploreprob>" << _fExploreProb << "</exploreprob>" << endl;
        O << "<expectedsize>" << _nExpectedDataSize << "</expectedsize>" << endl;
        return !!O;
    }
 
    // called at the end of every XML tag, _ss contains the data 
    virtual bool endElement(const std::string& name)
    {
        // _ss is an internal stringstream that holds the data of the tag
        if( name == "exploreprob")
            _ss >> _fExploreProb;
        else if( name == "expectedsize" )
            _ss >> _nExpectedDataSize;
        else // give a chance for the default parameters to get processed
            return PlannerParameters::endElement(name);
        return false;
    }
};

class RAStarParameters : public PlannerBase::PlannerParameters
{
 public:
 RAStarParameters() : fRadius(0.1f), fDistThresh(0.03f), fGoalCoeff(1), nMaxChildren(5), nMaxSampleTries(10) {}
        
    dReal fRadius;      ///< _pDistMetric thresh is the radius that children must be within parents
    dReal fDistThresh;  ///< gamma * _pDistMetric->thresh is the sampling radius
    dReal fGoalCoeff;   ///< balancees exploratino vs cost
    int nMaxChildren;   ///< limit on number of children
    int nMaxSampleTries; ///< max sample tries before giving up on creating a child
 protected:
    virtual bool serialize(std::ostream& O) const
    {
        if( !PlannerParameters::serialize(O) )
            return false;

        O << "<radius>" << fRadius << "</radius>" << endl;
        O << "<distthresh>" << fDistThresh << "</distthresh>" << endl;
        O << "<goalcoeff>" << fGoalCoeff << "</goalcoeff>" << endl;
        O << "<maxchildren>" << nMaxChildren << "</maxchildren>" << endl;
        O << "<maxsampletries>" << nMaxSampleTries << "</maxsampletries>" << endl;
    
        return !!O;
    }
        
    virtual bool endElement(const string& name)
    {
        if( name == "radius")
            _ss >> fRadius;
        else if( name == "distthresh")
            _ss >> fDistThresh;
        else if( name == "goalcoeff")
            _ss >> fGoalCoeff;
        else if( name == "maxchildren")
            _ss >> nMaxChildren;
        else if( name == "maxsampletries")
            _ss >> nMaxSampleTries;
        else
            return PlannerParameters::endElement(name);
        return false;
    }
};


#ifdef RAVE_REGISTER_BOOST
#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

BOOST_TYPEOF_REGISTER_TYPE(SimpleNode)
BOOST_TYPEOF_REGISTER_TYPE(SpatialTree)

#endif

#endif