#ifndef OPENRAVE_VISUALSERVOING_PROBLEM
#define OPENRAVE_VISUALSERVOING_PROBLEM

struct FRUSTUM
{
    Vector right, up, dir, pos;
    dReal fnear, ffar;
    dReal ffovx,ffovy;
    dReal fcosfovx,fsinfovx,fcosfovy,fsinfovy;

    void Init(const float* pKK,int width, int height)
    {
        ffovx = atanf(0.5f*width/pKK[0]);
        fcosfovx = cosf(ffovx); fsinfovx = sinf(ffovx);
        ffovy = atanf(0.5f*height/pKK[1]);
        fcosfovy = cosf(ffovy); fsinfovy = sinf(ffovy);
    }
};

inline bool IsOBBinFrustum(const OBB& o, const FRUSTUM& fr)
{
	// check OBB against all 6 planes
	Vector v = o.pos - fr.pos;

	// if v lies on the left or bottom sides of the frustrum
	// then freflect about the planes to get it on the right and 
	// top sides

	// side planes
	Vector vNorm = fr.fcosfovx * fr.right - fr.fsinfovx * fr.dir;
    if( dot3(v,vNorm) > -o.extents.x * RaveFabs(dot3(vNorm, o.right)) - 
        o.extents.y * RaveFabs(dot3(vNorm, o.up)) - 
        o.extents.z * RaveFabs(dot3(vNorm, o.dir)))
        return false;
    
	vNorm = -fr.fcosfovx * fr.right - fr.fsinfovx * fr.dir;
	if(dot3(v, vNorm) > -o.extents.x * RaveFabs(dot3(vNorm, o.right)) -
				o.extents.y * RaveFabs(dot3(vNorm, o.up)) -
				o.extents.z * RaveFabs(dot3(vNorm, o.dir))) return false;

	vNorm = fr.fcosfovy * fr.up - fr.fsinfovy * fr.dir;
	if(dot3(v, vNorm) > -o.extents.x * RaveFabs(dot3(vNorm, o.right)) -
				o.extents.y * RaveFabs(dot3(vNorm, o.up)) -
				o.extents.z * RaveFabs(dot3(vNorm, o.dir))) return false;

	vNorm = -fr.fcosfovy * fr.up - fr.fsinfovy * fr.dir;
	if(dot3(v, vNorm) > -o.extents.x * RaveFabs(dot3(vNorm, o.right)) -
				o.extents.y * RaveFabs(dot3(vNorm, o.up)) -
				o.extents.z * RaveFabs(dot3(vNorm, o.dir))) return false;

	vNorm.x = dot3(v, fr.dir);
	vNorm.y = o.extents.x * RaveFabs(dot3(fr.dir, o.right)) + 
					o.extents.y * RaveFabs(dot3(fr.dir, o.up)) + 
					o.extents.z * RaveFabs(dot3(fr.dir, o.dir));

	if( (vNorm.x < fr.fnear + vNorm.y) || (vNorm.x > fr.ffar - vNorm.y) ) return false;

	return true;
}

/// returns true if all points on the oriented bounding box are inside the convex hull
/// planes should be facing inside
inline bool IsOBBinConvexHull(const OBB& o, const vector<Vector>& vplanes)
{
    FOREACH(itplane, vplanes) {
        // side planes
        if( dot3(o.pos,*itplane)+itplane->w < o.extents.x * RaveFabs(dot3(*itplane, o.right))
            + o.extents.y * RaveFabs(dot3(*itplane, o.up))
            + o.extents.z * RaveFabs(dot3(*itplane, o.dir)))
            return false;
    }

    return true;
}

/// samples rays from the projected OBB and returns true if the test function returns true
/// for all the rays. Otherwise, returns false
template <class TestFn>
bool SampleProjectedOBBWithTest(const OBB& obb, dReal delta, const TestFn testfn)
{
    dReal fscalefactor = 0.95f; // have to make box smaller or else rays might miss
    Vector vpoints[8] = {obb.pos + fscalefactor*(obb.right*obb.extents.x + obb.up*obb.extents.y + obb.dir*obb.extents.z),
                         obb.pos + fscalefactor*(obb.right*obb.extents.x + obb.up*obb.extents.y - obb.dir*obb.extents.z),
                         obb.pos + fscalefactor*(obb.right*obb.extents.x - obb.up*obb.extents.y + obb.dir*obb.extents.z),
                         obb.pos + fscalefactor*(obb.right*obb.extents.x - obb.up*obb.extents.y - obb.dir*obb.extents.z),
                         obb.pos + fscalefactor*(-obb.right*obb.extents.x + obb.up*obb.extents.y + obb.dir*obb.extents.z),
                         obb.pos + fscalefactor*(-obb.right*obb.extents.x + obb.up*obb.extents.y - obb.dir*obb.extents.z),
                         obb.pos + fscalefactor*(-obb.right*obb.extents.x - obb.up*obb.extents.y + obb.dir*obb.extents.z),
                         obb.pos + fscalefactor*(-obb.right*obb.extents.x - obb.up*obb.extents.y - obb.dir*obb.extents.z)};
//    Vector vpoints3d[8];
//    for(int j = 0; j < 8; ++j) vpoints3d[j] = tcamera*vpoints[j];

    for(int i =0; i < 8; ++i) {
        dReal fz = 1.0f/vpoints[i].z;
        vpoints[i].x *= fz;
        vpoints[i].y *= fz;
        vpoints[i].z = 1;
    }

    int faceindices[3][4];
    if( obb.right.z >= 0 ) {
        faceindices[0][0] = 4; faceindices[0][1] = 5; faceindices[0][2] = 6; faceindices[0][3] = 7;
    }
    else {
        faceindices[0][0] = 0; faceindices[0][1] = 1; faceindices[0][2] = 2; faceindices[0][3] = 3;
    }
    if( obb.up.z >= 0 ) {
        faceindices[1][0] = 2; faceindices[1][1] = 3; faceindices[1][2] = 6; faceindices[1][3] = 7;
    }
    else {
        faceindices[1][0] = 0; faceindices[1][1] = 1; faceindices[1][2] = 4; faceindices[1][3] = 5;
    }
    if( obb.dir.z >= 0 ) {
        faceindices[2][0] = 1; faceindices[2][1] = 3; faceindices[2][2] = 5; faceindices[2][3] = 7;
    }
    else {
        faceindices[2][0] = 0; faceindices[2][1] = 2; faceindices[2][2] = 4; faceindices[2][3] = 6;
    }

    for(int i = 0; i < 3; ++i) {
        Vector v0 = vpoints[faceindices[i][0]];
        Vector v1 = vpoints[faceindices[i][1]]-v0;
        Vector v2 = vpoints[faceindices[i][2]]-v0;
        Vector v3 = vpoints[faceindices[i][3]]-v0;
        dReal f3length = RaveSqrt(v3.lengthsqr2());
        Vector v3norm = v3 * (1.0f/f3length);
        Vector v3perp(-v3norm.y,v3norm.x,0,0);
        dReal f1proj = RaveFabs(dot2(v3perp,v1)), f2proj = RaveFabs(dot2(v3perp,v2));
        
        int n1 = f1proj/delta;
        dReal n1scale = 1.0f/n1;
        Vector vdelta1 = v1*n1scale;
        Vector vdelta2 = (v1-v3)*n1scale;
        dReal fdeltalen = (RaveFabs(dot2(v3norm,v1)) + RaveFabs(dot2(v3norm,v1-v3)))*n1scale;
        dReal ftotalen = f3length;
        Vector vcur1 = v0, vcur2 = v0+v3;
        for(int j = 0; j <= n1; ++j, vcur1 += vdelta1, vcur2 += vdelta2, ftotalen -= fdeltalen ) {
            int numsteps = ftotalen/delta;
            Vector vdelta = (vcur2-vcur1)*(1.0f/numsteps), vcur = vcur1;
            for(int k = 0; k <= numsteps; ++k, vcur += vdelta) {
                if( !testfn(vcur) )
                    return false;
            }
        }
        
//        Vector vtripoints[6] = {vpoints3d[faceindices[i][0]], vpoints3d[faceindices[i][3]], vpoints3d[faceindices[i][1]],
//                                vpoints3d[faceindices[i][0]], vpoints3d[faceindices[i][1]], vpoints3d[faceindices[i][3]]};
//        penv->drawtrimesh(vtripoints[0], 16, NULL, 2);

        int n2 = f2proj/delta;
        if( n2 == 0 )
            continue;

        dReal n2scale = 1.0f/n2;
        vdelta1 = v2*n2scale;
        vdelta2 = (v2-v3)*n2scale;
        fdeltalen = (RaveFabs(dot2(v3norm,v2)) + RaveFabs(dot2(v3norm,v2-v3)))*n2scale;
        ftotalen = f3length;
        vcur1 = v0; vcur2 = v0+v3;
        vcur1 += vdelta1; vcur2 += vdelta2; ftotalen -= fdeltalen; // do one step
        for(int j = 0; j < n2; ++j, vcur1 += vdelta1, vcur2 += vdelta2, ftotalen -= fdeltalen ) {
            int numsteps = ftotalen/delta;
            Vector vdelta = (vcur2-vcur1)*(1.0f/numsteps), vcur = vcur1;
            for(int k = 0; k <= numsteps; ++k, vcur += vdelta) {
                if( !testfn(vcur) )
                    return false;
            }
        }
    }

    return true;
}

class VisualFeedbackProblem : public CmdProblemInstance
{
public:
    struct ASSEMBLYOBJECT
    {
        string filename;
        dReal _fAppearProb;
    };

    class VisibilityConstraintFunction : public PlannerBase::ConstraintFunction
    {
    public:
        VisibilityConstraintFunction(RobotBase* probot, KinBody* ptarget, const string& convexfilename, int sensorindex=0) : _probot(probot), _ptarget(ptarget), _sensorindex(sensorindex) {
            _psensor = &_probot->GetSensors().at(_sensorindex);
            if( _psensor->GetSensor() == NULL ) {
                RAVELOG_ERRORA("no vaild sensor attached\n");
                throw;
            }
            if( _psensor->GetSensor()->GetSensorGeometry()->GetType() != SensorBase::ST_Camera) {
                RAVELOG_ERRORA("sensor is not a camera\n");
                throw;
            }

            // check if there is a manipulator with the same end effector as camera
            _pmanip = NULL;
            FOREACHC(itmanip,_probot->GetManipulators()) {
                if( itmanip->pEndEffector == _psensor->GetAttachingLink() ) {
                    _pmanip = &(*itmanip);
                    break;
                }
            }

            if( _pmanip == NULL ) {
                RAVELOG_ERRORA("failed to find manipulator with end effector similar to sensor link\n");
                throw;
            }

            _tlocalsensorinv = _psensor->GetRelativeTransform().inverse();
            SensorBase::CameraGeomData *pgeom = (SensorBase::CameraGeomData*)_psensor->GetSensor()->GetSensorGeometry();

            // get all child links of the camera
            _vChildLinks.push_back(make_pair(_psensor->GetAttachingLink(),_tlocalsensorinv));
            Transform tbaseinv = _tlocalsensorinv*_psensor->GetAttachingLink()->GetTransform().inverse();
            int iattlink = _psensor->GetAttachingLink()->GetIndex();
            FOREACH(itlink, _probot->GetLinks()) {
                int ilink = (*itlink)->GetIndex();
                if( ilink == _psensor->GetAttachingLink()->GetIndex() )
                    continue;

                // have to be affected by arm
                if( _pmanip->_vecarmjoints.size() > 0 && !_probot->DoesAffect(_pmanip->_vecarmjoints[0],ilink) )
                    continue;
                for(int ijoint = 0; ijoint < _probot->GetDOF(); ++ijoint) {
                    if( _probot->DoesAffect(ijoint,ilink) && !_probot->DoesAffect(ijoint,iattlink) ) {
                        RAVELOG_DEBUGA("child link %S, joint: %S\n", (*itlink)->GetName(), _probot->GetJoints()[ijoint]->GetName() );
                        _vChildLinks.push_back(make_pair(*itlink,tbaseinv*(*itlink)->GetTransform()));
                        break;
                    }
                }
            }

            _vTargetOBBs.reserve(_ptarget->GetLinks().size());
            FOREACHC(itlink, _ptarget->GetLinks())
                _vTargetOBBs.push_back(OBBFromAABB((*itlink)->GetCollisionData().ComputeAABB(),(*itlink)->GetTransform()));
            _abTarget = _ptarget->ComputeAABB();

            // create the dummy box
            {
                KinBody::KinBodyStateSaver saver(_ptarget);
                _ptarget->SetTransform(Transform());
                vector<AABB> vboxes; vboxes.push_back(_ptarget->ComputeAABB());

                //_ptarget->GetEnv()->LockPhysics(true);
                _ptargetbox.reset(_ptarget->GetEnv()->CreateKinBody());
                _ptargetbox->InitFromBoxes(vboxes,false);
                _ptargetbox->GetEnv()->AddKinBody(_ptargetbox.get());
                _ptargetbox->Enable(false);
                _ptargetbox->SetTransform(_ptarget->GetTransform());
                _ptargetbox->SetName(L"dummytarget");
                //_ptarget->GetEnv()->LockPhysics(false);
            }

            if( convexfilename.size() > 0 ) {
                vector<Vector> vpoints;
                Vector vprev,vprev2,v,vdir,vnorm,vcenter;
                ifstream f(convexfilename.c_str());
                if( !f )
                    throw string("failed to open convex filename");
                while(!f.eof()) {
                    f >> v.x >> v.y;
                    if( !f )
                        break;
                    vpoints.push_back(Vector((v.x-pgeom->KK[2])/pgeom->KK[0],(v.y-pgeom->KK[3])/pgeom->KK[1],0,0));
                    vcenter += vpoints.back();
                }
                
                if( vpoints.size() > 2 ) {
                    vcenter *= 1.0f/vpoints.size();

                    // get the planes
                    _vconvexplanes.reserve(vpoints.size());
                    vprev = vpoints.back();
                    FOREACH(itv, vpoints) {
                        vdir = *itv-vprev;
                        vnorm.x = vdir.y;
                        vnorm.y = -vdir.x;
                        // normal has to be facing inside
                        if( dot2(vnorm,vcenter-vprev) < 0 ) {
                            vnorm = -vnorm;
                        }
                        vnorm.z = -(vnorm.x*vprev.x+vnorm.y*vprev.y);
                        _vconvexplanes.push_back(vnorm.normalize3());
                        vprev = *itv;
                    }

                    // get the center of the convex hull
                    dReal totalarea = 0;
                    _vcenterconvex = Vector(0,0,0);
                    for(size_t i = 2; i < vpoints.size(); ++i) {
                        Vector v0 = vpoints[i-1]-vpoints[0];
                        Vector v1 = vpoints[i]-vpoints[0];
                        dReal area = RaveFabs(v0.x*v1.y-v0.y*v1.x);
                        _vcenterconvex += area*(vpoints[0]+vpoints[i-1]+vpoints[i-1]);
                        totalarea += area;
                    }
                    _vcenterconvex /= 3.0f*totalarea; _vcenterconvex.z = 1;
                }
                else
                    RAVELOG_WARNA("convex file %s does not have enough points\n",convexfilename.c_str());
            }
            
            if( _vconvexplanes.size() == 0 ) {
                // pick the camera boundaries
                _vconvexplanes.push_back(Vector(1,0,pgeom->KK[2]/pgeom->KK[0],0).normalize3()); // -x
                _vconvexplanes.push_back(Vector(-1,0,-(pgeom->width-pgeom->KK[2])/pgeom->KK[0],0).normalize3()); // +x
                _vconvexplanes.push_back(Vector(1,0,pgeom->KK[3]/pgeom->KK[1],0).normalize3()); // -y
                _vconvexplanes.push_back(Vector(-1,0,-(pgeom->height-pgeom->KK[3])/pgeom->KK[1],0).normalize3()); // +y
                _vcenterconvex = Vector(0,0,1);
            }

            _ttarget = _ptarget->GetTransform();
            _fSampleRayDensity = 20.0f/pgeom->KK[0];
        }
        virtual ~VisibilityConstraintFunction() {
            //_ptargetbox->GetEnv()->LockPhysics(true);
            _ptargetbox->GetEnv()->RemoveKinBody(_ptargetbox.get(),false);
            //_ptargetbox->GetEnv()->LockPhysics(false);
        }
        
        virtual bool Constraint(const dReal* pSrcConf, dReal* pDestConf, Transform* ptrans, int settings)
        {
            TransformMatrix tcamera = _psensor->GetSensor()->GetTransform();
            if( !InConvexHull(tcamera) )
                return false;
            // no need to check gripper collision

            bool bOcclusion = IsOccluded(tcamera);
            if( !bOcclusion )
                memcpy(pDestConf,pSrcConf,sizeof(dReal)*_probot->GetActiveDOF());
            return !bOcclusion;
        }

        bool SampleWithCamera(const TransformMatrix& tcamera, dReal* pNewSample)
        {
            if( !InConvexHull(tcamera) )
                return false;
            if( IsGripperCollision(tcamera) )
                return false;
            
            // object is inside, find an ik solution
            Transform tgoalee = tcamera*_tlocalsensorinv*_pmanip->tGrasp;
            if( !_pmanip->FindIKSolution(tgoalee,_vsolution,true) ) {
                RAVELOG_VERBOSEA("no valid ik\n");
                return false;
            }

            // convert the solution into active dofs
            _probot->GetActiveDOFValues(pNewSample);
            FOREACH(itarm,_pmanip->_vecarmjoints) {
                vector<int>::const_iterator itactive = find(_probot->GetActiveJointIndices().begin(),_probot->GetActiveJointIndices().end(),*itarm);
                if( itactive != _probot->GetActiveJointIndices().end() )
                    pNewSample[(int)(itactive-_probot->GetActiveJointIndices().begin())] = _vsolution[(int)(itarm-_pmanip->_vecarmjoints.begin())];
            }
            _probot->SetActiveDOFValues(NULL,pNewSample);
            
            return !IsOccluded(tcamera);
        }

        bool InConvexHull(const TransformMatrix& tcamera)
        {
            Vector vitrans(-tcamera.m[0]*tcamera.trans.x - tcamera.m[4]*tcamera.trans.y - tcamera.m[8]*tcamera.trans.z,
                           -tcamera.m[1]*tcamera.trans.x - tcamera.m[5]*tcamera.trans.y - tcamera.m[9]*tcamera.trans.z,
                           -tcamera.m[2]*tcamera.trans.x - tcamera.m[6]*tcamera.trans.y - tcamera.m[10]*tcamera.trans.z);
            _vconvexplanes3d.resize(_vconvexplanes.size());
            for(size_t i = 0; i < _vconvexplanes.size(); ++i) {
                _vconvexplanes3d[i] = tcamera.rotate(_vconvexplanes[i]);
                _vconvexplanes3d[i].w = dot3(vitrans,_vconvexplanes[i]);
            }

            FOREACH(itobb,_vTargetOBBs) {
                if( !IsOBBinConvexHull(*itobb,_vconvexplanes3d) ) {
                    RAVELOG_VERBOSEA("box not in camera vision hull\n");
                    return false;
                }
            }
            
            return true;
        }

        /// check if any part of the environment or robot is in front of the camera blocking the object
        /// sample object's surface and shoot rays
        bool IsOccluded(const TransformMatrix& tcamera)
        {
            TransformMatrix tcamerainv = tcamera.inverse();
            _ptargetbox->Enable(true);
            _ptarget->Enable(false);
            _ptargetbox->SetTransform(_ptarget->GetTransform());
            bool bOcclusion = false;
            FOREACH(itobb,_vTargetOBBs) {
                OBB cameraobb = TransformOBB(*itobb,tcamerainv);
                if( !SampleProjectedOBBWithTest(cameraobb, _fSampleRayDensity, boost::bind(&VisibilityConstraintFunction::TestRay, this, _1, boost::ref(tcamera))) ) {
                    bOcclusion = true;
                    RAVELOG_VERBOSEA("box is occluded\n");
                    break;
                }
            }
            _ptargetbox->Enable(false);
            _ptarget->Enable(true);
            return bOcclusion;
        }

        bool IsGripperCollision(const Transform& tcamera)
        {
            FOREACH(itlink,_vChildLinks) {
                itlink->first->SetTransform(tcamera*itlink->second);
                if( _probot->GetEnv()->CheckCollision(itlink->first) )
                    return true;

            }
            return false;
        }

        bool TestRay(const Vector& v, const TransformMatrix& tcamera)
        {
            RAY r;
            r.dir = tcamera.rotate(v*(1.0f/RaveSqrt(v.lengthsqr3())));
            r.pos = tcamera.trans + 0.05f*r.dir; // move the rays a little forward
            if( !_probot->GetEnv()->CheckCollision(r,&_report) ) {
                RAVELOG_DEBUGA("no collision!?\n");
                return true; // not supposed to happen, but it is OK
            }

//            RaveVector<float> vpoints[2];
//            vpoints[0] = r.pos;
//            assert(_report.contacts.size() == 1 );
//            vpoints[1] = _report.contacts[0].pos;
//            _probot->GetEnv()->drawlinestrip(vpoints[0],2,16,1.0f,Vector(0,0,1));
            if( !(_report.plink1 != NULL && _report.plink1->GetParent() == _ptargetbox.get()) )
                RAVELOG_DEBUGA("bad collision: %S:%S\n", _report.plink1->GetParent()->GetName(), _report.plink1->GetName());
            return _report.plink1 != NULL && _report.plink1->GetParent() == _ptargetbox.get();
        }

        inline RobotBase* GetRobot() const { return _probot; }
        inline Vector GetConvexCenter() const { return _vcenterconvex; }
        inline RobotBase::Manipulator* GetManipulator() const { return _pmanip; }
        inline KinBody* GetTarget() const { return _ptarget; }

    private:
        RobotBase* _probot;
        KinBody* _ptarget;
        boost::shared_ptr<KinBody> _ptargetbox; ///< box to represent the target for simulating ray collisions
        Transform _ttarget; ///< transform of target
        Transform _tlocalsensorinv; ///< local sensor transform (with respect to link)
        RobotBase::AttachedSensor* _psensor;
        RobotBase::Manipulator* _pmanip;
        vector<pair<KinBody::Link*,Transform> > _vChildLinks; ///< all child links of the links attached to the camera
        int _sensorindex;
        vector<OBB> _vTargetOBBs; // object links local AABBs
        vector<dReal> _vsolution;
        COLLISIONREPORT _report;
        AABB _abTarget; // target aabb
        dReal _fSampleRayDensity;
        
        vector<Vector> _vconvexplanes, _vconvexplanes3d; // the planes defining the bounding visibility region (posive is inside)
        Vector _vcenterconvex; // center point on the z=1 plane of the convex region
    };

    class GoalSampleFunction : public PlannerBase::SampleFunction
    {
    public:
        GoalSampleFunction(RobotBase* probot, KinBody* ptarget, const string& convexfilename, const string& visibilityfilename, bool bVisibilityTransforms, int sensorindex=0) : _vconstraint(probot,ptarget,convexfilename,sensorindex)
        {
            {
                KinBody::KinBodyStateSaver saver(_vconstraint.GetTarget());
                _ttarget = _vconstraint.GetTarget()->GetTransform();
                _vconstraint.GetTarget()->SetTransform(Transform());
                _vTargetLocalCenter = _vconstraint.GetTarget()->ComputeAABB().pos;
            }

            int nNumRolls = 8;
            dReal deltaroll = PI*2.0f/(dReal)nNumRolls;

            if( visibilityfilename.size() == 0 ) {
                vector<Vector> vspheredirs;
                GenerateSphereTriangulation(vspheredirs,3);
                int nNumDists = 5;
                dReal fmindist = 0.2f, fdeltadist = 0.06f;
                _vcameras.resize(vspheredirs.size()*nNumDists*nNumRolls);
                Transform* pcamera = &_vcameras[0];
                
                for(size_t j = 0; j < vspheredirs.size(); ++j) {
                    Vector v = vspheredirs[j];
                    for(int i = 0; i < nNumDists; ++i) {
                        dReal froll = 0;
                        for(int iroll = 0; iroll < nNumRolls; ++iroll, froll += deltaroll) {
                            *pcamera++ = ComputeCameraMatrix(vspheredirs[j],fmindist + fdeltadist*i,froll,Vector());
                        }
                    }
                }
            }
            else {
                fstream fextents(visibilityfilename.c_str());
                if( !fextents )
                    throw string("failed to open visiblity file");

                _vcameras.reserve(400);

                if( bVisibilityTransforms ) {
                    Transform t;
                    while(!fextents.eof()) {
                        fextents >> t;
                        if( !fextents )
                            break;
                        _vcameras.push_back(t);
                    }
                }
                else {
                    Vector v;
                    while(!fextents.eof()) {
                        fextents >> v.x >> v.y >> v.z;
                        if( !fextents )
                            break;
                        dReal fdist = RaveSqrt(v.lengthsqr3());
                        v *= 1.0f/fdist;

                        dReal froll = 0;
                        for(int iroll = 0; iroll < nNumRolls; ++iroll, froll += deltaroll)
                            _vcameras.push_back(ComputeCameraMatrix(v,fdist,froll,Vector()));
                    }
                }
            }
            
            RAVELOG_DEBUGA("have %"PRIdS" detection extents hypotheses\n", _vcameras.size());
            _sphereperms.PermuteStart(_vcameras.size());
        }
        virtual ~GoalSampleFunction() {
        }

        virtual void Sample(dReal* pNewSample)
        {
            RobotBase::RobotStateSaver state(_vconstraint.GetRobot());

            if( _sphereperms.PermuteContinue(boost::bind(&GoalSampleFunction::SampleWithParameters,this,_1,pNewSample)) >= 0 ) {
                return;
            }

            // start from the beginning, if nothing, throw
            _sphereperms.PermuteStart(_vcameras.size());
            if( _sphereperms.PermuteContinue(boost::bind(&GoalSampleFunction::SampleWithParameters,this,_1,pNewSample)) >= 0 ) {
                return;
            }

            throw string("failed to get new sample!!\n");
        }

        virtual bool Sample(dReal* pNewSample, const dReal* pCurSample, dReal fRadius)
        {
            Sample(pNewSample);
            return true;
        }

        bool SampleWithParameters(int isample, dReal* pNewSample)
        {
            TransformMatrix tcamera = _ttarget*_vcameras[isample];
            return _vconstraint.SampleWithCamera(tcamera,pNewSample);
        }

        inline RobotBase::Manipulator* GetManipulator() const { return _vconstraint.GetManipulator(); }

        const vector<Transform>& GetCameraTransforms() const { return _vcameras; }
        VisibilityConstraintFunction _vconstraint;

        TransformMatrix ComputeCameraMatrix(const Vector& vdir,dReal fdist,dReal froll,Vector xyoffset)
        {
            Vector vright, vup = Vector(0,1,0) - vdir * vdir.y;
            dReal uplen = vup.lengthsqr3();
            if( uplen < 0.001 ) {
                vup = Vector(0,0,1) - vdir * vdir.z;
                uplen = vup.lengthsqr3();
            }
            
            vup *= (dReal)1.0/RaveSqrt(uplen);
            cross3(vright,vup,vdir);
            TransformMatrix tcamera;
            tcamera.m[2] = vdir.x; tcamera.m[6] = vdir.y; tcamera.m[10] = vdir.z;

            dReal fcosroll = RaveCos(froll), fsinroll = RaveSin(froll);
            tcamera.m[0] = vright.x*fcosroll+vup.x*fsinroll; tcamera.m[1] = -vright.x*fsinroll+vup.x*fcosroll;
            tcamera.m[4] = vright.y*fcosroll+vup.y*fsinroll; tcamera.m[5] = -vright.y*fsinroll+vup.y*fcosroll;
            tcamera.m[8] = vright.z*fcosroll+vup.z*fsinroll; tcamera.m[9] = -vright.z*fsinroll+vup.z*fcosroll;
            tcamera.trans = _vTargetLocalCenter - fdist * tcamera.rotate(_vconstraint.GetConvexCenter());
            return tcamera;
        }

    private:
        Transform _ttarget;
        Vector _vTargetLocalCenter;
        RandomPermuationExecutor _sphereperms;
        vector<Transform> _vcameras; ///< camera transformations in local coord systems
    };

    VisualFeedbackProblem(EnvironmentBase* penv) : CmdProblemInstance(penv)
    {
        _fAssemblySpeed = 0;
        RegisterCommand("ProcessVisibilityExtents",(CommandFn)&VisualFeedbackProblem::ProcessVisibilityExtents,"Processes the visibility directions for containment of the object inside the gripper mask");
        RegisterCommand("ComputeVisibility",(CommandFn)&VisualFeedbackProblem::ComputeVisibility,"Computes the visibility of the current robot configuration");
        RegisterCommand("SampleVisibilityGoal",(CommandFn)&VisualFeedbackProblem::SampleVisibilityGoal,"Samples a goal maintaining camera visibility constraints");
        RegisterCommand("MoveToObserveTarget",(CommandFn)&VisualFeedbackProblem::MoveToObserveTarget, "Approaches a target object while choosing a goal such that the robot's camera sensor sees the object ");
        RegisterCommand("VisualFeedbackGrasping",(CommandFn)&VisualFeedbackProblem::VisualFeedbackGrasping,"Stochastic greedy grasp planner considering visibility");
        RegisterCommand("AssemblyLine",(CommandFn)&VisualFeedbackProblem::AssemblyLine,"");
        RegisterCommand("TestIK",(CommandFn)&VisualFeedbackProblem::TestIK,"");
        RegisterCommand("help", (CommandFn)&VisualFeedbackProblem::Help,"Help message");
    }

    ~VisualFeedbackProblem() {
        FOREACH(it, _listAssemblyObjects)
            GetEnv()->RemoveKinBody(it->get(),false);
        _listAssemblyObjects.clear();
    }

    void Destroy() {
        RAVELOG_INFOA("problem unloaded from environment\n");
    }

    int main(const char* cmd)
    {
        if( cmd == NULL )
            return 0;

        const char* delim = " \r\n\t";
        string mycmd = cmd;
        char* p = strtok(&mycmd[0], delim);
        if( p != NULL )
            _strRobotName = _stdmbstowcs(p);
    
        SetActiveRobots(GetEnv()->GetRobots()); 
        return 0;
    }

    void SetActiveRobots(const std::vector<RobotBase*>& robots)
    {
        _probot = NULL;

        if( robots.size() == 0 ) {
            RAVELOG_WARNA("No robots to plan for\n");
            return;
        }

        vector<RobotBase*>::const_iterator itrobot;
        FORIT(itrobot, robots) {
            if( wcsicmp((*itrobot)->GetName(), _strRobotName.c_str() ) == 0  ) {
                _probot = *itrobot;
                break;
            }
        }

        if( _probot == NULL ) {
            RAVELOG_ERRORA("Failed to find %S\n", _strRobotName.c_str());
            return;
        }
    }

    bool SendCommand(const char* cmd, string& response)
    {
        //LockEnvironment envlock(GetEnv());
        SetActiveRobots(GetEnv()->GetRobots());
        if( _probot == NULL ) {
            RAVELOG_ERRORA("robot is NULL, send command failed\n");
            return false;
        }
        return CmdProblemInstance::SendCommand(cmd, response);
    }

    bool ProcessVisibilityExtents(ostream& sout, istream& sinput)
    {
        string cmd;

        int sensorindex=0;
        KinBody* ptarget = NULL;
        string convexfilename,visibilityfile;
        while(!sinput.eof()) {
            sinput >> cmd;
            if( !sinput )
                break;
        
            if( cmd == "target" ) {
                string name; sinput >> name;
                ptarget = GetEnv()->GetKinBody(_stdmbstowcs(name.c_str()).c_str());
            }
            else if( cmd == "sensorindex" )
                sinput >> sensorindex;
            else if( cmd == "convexfile" )
                sinput >> convexfilename;
            else if( cmd == "visibilityfile" )
                sinput >> visibilityfile;
            else break;

            if( !sinput ) {
                RAVELOG_ERRORA("failed\n");
                return false;
            }
        }

        if( ptarget == NULL )
            return false;

        KinBody::KinBodyStateSaver saver(ptarget);
        ptarget->SetTransform(Transform());

        boost::shared_ptr<GoalSampleFunction> pgoalfn;
        try {
            pgoalfn.reset(new GoalSampleFunction(_probot,ptarget,convexfilename, visibilityfile, false, sensorindex));
        }

        catch(const string& err) {
            RAVELOG_ERRORA(err.c_str());
            return false;
        }

        // get all the camera positions and test them
        FOREACHC(itcamera, pgoalfn->GetCameraTransforms()) {
            if( pgoalfn->_vconstraint.InConvexHull(*itcamera) )
                sout << *itcamera << " ";
        }

        return true;
    }

    bool ComputeVisibility(ostream& sout, istream& sinput)
    {
        string cmd;

        int sensorindex=0;
        KinBody* ptarget = NULL;
        string convexfilename;
        while(!sinput.eof()) {
            sinput >> cmd;
            if( !sinput )
                break;
        
            if( cmd == "target" ) {
                string name; sinput >> name;
                ptarget = GetEnv()->GetKinBody(_stdmbstowcs(name.c_str()).c_str());
            }
            else if( cmd == "sensorindex" )
                sinput >> sensorindex;
            else if( cmd == "convexfile" )
                sinput >> convexfilename;
            else break;

            if( !sinput ) {
                RAVELOG_ERRORA("failed\n");
                return false;
            }
        }

        boost::shared_ptr<VisibilityConstraintFunction> pconstraintfn;
        try {
            pconstraintfn.reset(new VisibilityConstraintFunction(_probot,ptarget,convexfilename, sensorindex));
        }
        catch(const string& err) {
            RAVELOG_ERRORA(err.c_str());
            return false;
        }

        vector<dReal> v;
        _probot->GetActiveDOFValues(v);
        sout << pconstraintfn->Constraint(&v[0],&v[0],NULL,0);
        return true;
    }

    bool SampleVisibilityGoal(ostream& sout, istream& sinput)
    {
        string cmd;

        int sensorindex=0;
        KinBody* ptarget = NULL;
        string convexfilename,visibilityfilename;
        int numsamples=1;
        bool bVisibilityTransforms=false;
        while(!sinput.eof()) {
            sinput >> cmd;
            if( !sinput )
                break;
        
            if( cmd == "target" ) {
                string name; sinput >> name;
                ptarget = GetEnv()->GetKinBody(_stdmbstowcs(name.c_str()).c_str());
            }
            else if( cmd == "sensorindex" )
                sinput >> sensorindex;
            else if( cmd == "convexfile" )
                sinput >> convexfilename;
            else if( cmd == "samples" )
                sinput >> numsamples;
            else if( cmd == "visibilityfile")
                sinput >> visibilityfilename;
            else if( cmd == "visibilitytrans") {
                sinput >> visibilityfilename;
                bVisibilityTransforms = true;
            }
            else break;

            if( !sinput ) {
                RAVELOG_ERRORA("failed\n");
                return false;
            }
        }

        boost::shared_ptr<GoalSampleFunction> pgoalsampler;

        try {
            pgoalsampler.reset(new GoalSampleFunction(_probot,ptarget,convexfilename,visibilityfilename,bVisibilityTransforms,sensorindex));
     
            uint64_t starttime = GetMicroTime();
            vector<dReal> vsamples(_probot->GetActiveDOF()*numsamples);
            for(int i = 0; i < numsamples; ++i) {
                pgoalsampler->Sample(&vsamples[i*_probot->GetActiveDOF()]);
            }

            float felapsed = (GetMicroTime()-starttime)*1e-6f;

            RAVELOG_INFOA("total time for %d samples is %fs, %f avg\n", numsamples,felapsed,felapsed/numsamples);
            FOREACH(it,vsamples)
                sout << *it << " ";
            sout << felapsed;
        }
        catch(const string& err) {
            RAVELOG_ERRORA(err.c_str());
            return false;
        }

        return true;
    }

    bool MoveToObserveTarget(ostream& sout, istream& sinput)
    {
        string strtrajfilename;
        bool bExecute = true, bOutputTraj = false;;

        VisualApproachParameters params;
        params.nMaxIterations = 4000;
        int affinedofs=0, sensorindex=0;
        KinBody* ptarget = NULL;
        string cmd, plannername="GoalSampler",convexfilename,visibilityfilename;
        bool bVisibilityTransforms=false;
        while(!sinput.eof()) {
            sinput >> cmd;
            if( !sinput )
                break;
        
            if( cmd == "outputtraj" )
                bOutputTraj = true;
            else if( cmd == "affinedofs" )
                sinput >> affinedofs;
            else if( cmd == "maxiter" )
                sinput >> params.nMaxIterations;
            else if( cmd == "execute" )
                sinput >> bExecute;
            else if( cmd == "writetraj" )
                sinput >> strtrajfilename;
            else if( cmd == "smoothpath" )
                sinput >> params._nSmoothPath;
            else if( cmd == "target" ) {
                string name; sinput >> name;
                ptarget = GetEnv()->GetKinBody(_stdmbstowcs(name.c_str()).c_str());
            }
            else if( cmd == "sensorindex" )
                sinput >> sensorindex;
            else if( cmd == "planner" )
                sinput >> plannername;
            else if( cmd == "sampleprob" )
                sinput >> params._fSampleGoalProb;
            else if( cmd == "convexfile" )
                sinput >> convexfilename;
            else if( cmd == "visibilityfile" )
                sinput >> visibilityfilename;
            else if( cmd == "visibilitytrans" ) {
                sinput >> visibilityfilename;
                bVisibilityTransforms = true;
            }
            else break;

            if( !sinput ) {
                RAVELOG_ERRORA("failed\n");
                return false;
            }
        }

        if( ptarget == NULL ) {
            RAVELOG_ERRORA("no target specified\n");
            return false;
        }

        boost::shared_ptr<GoalSampleFunction> pgoalsampler;

        try {
            pgoalsampler.reset(new GoalSampleFunction(_probot,ptarget,convexfilename,visibilityfilename,bVisibilityTransforms,sensorindex));
        }
        catch(const string& err) {
            RAVELOG_ERRORA(err.c_str());
            return false;
        }

        params._pgoals = pgoalsampler;
        _probot->RegrabAll();
        
        _probot->SetActiveDOFs(pgoalsampler->GetManipulator()->_vecarmjoints, affinedofs);
        _probot->GetActiveDOFValues(params.vinitialconfig);

        boost::shared_ptr<Trajectory> ptraj(GetEnv()->CreateTrajectory(_probot->GetActiveDOF()));

        Trajectory::TPOINT pt;
        pt.q = params.vinitialconfig;
        ptraj->AddPoint(pt);
    
        // jitter for initial collision
        if( !JitterActiveDOF(_probot) ) {
            RAVELOG_WARNA("jitter failed for initial\n");
            return false;
        }
        _probot->GetActiveDOFValues(params.vinitialconfig);

        // check if grasped 
        boost::shared_ptr<PlannerBase> planner(GetEnv()->CreatePlanner(plannername.c_str()));

        if( planner.get() == NULL ) {
            RAVELOG_ERRORA("failed to create BiRRTs\n");
            return false;
        }
    
        bool bSuccess = false;
        RAVELOG_INFOA("starting planning\n");
        uint64_t starttime = GetMicroTime();
        for(int iter = 0; iter < 1; ++iter) {
            if( !planner->InitPlan(_probot, &params) ) {
                RAVELOG_ERRORA("InitPlan failed\n");
                return false;
            }
        
            if( planner->PlanPath(ptraj.get()) ) {
                bSuccess = true;
                RAVELOG_INFOA("finished planning\n");
                break;
            }
            else RAVELOG_WARNA("PlanPath failed\n");
        }

        float felapsed = (GetMicroTime()-starttime)*0.000001f;
        RAVELOG_INFOA("total planning time: %fs\n", felapsed);
        if( !bSuccess )
            return false;

        SetTrajectory(ptraj.get(), bExecute, strtrajfilename, bOutputTraj?&sout:NULL);
        sout << felapsed;
        return true;
    }

    bool VisualFeedbackGrasping(ostream& sout, istream& sinput)
    {
        string strtrajfilename;
        bool bExecute = true, bOutputTraj = false;

        GraspSetParameters params(GetEnv());
        params.nMaxIterations = 4000;
        int sensorindex=0;
        KinBody* ptarget = NULL;
        string cmd, plannername="GraspGradient",convexfilename;
        params._fVisibiltyGraspThresh = 0.05f;
        dReal fMaxVelMult = 1;

        while(!sinput.eof()) {
            sinput >> cmd;
            if( !sinput )
                break;
        
            if( cmd == "outputtraj" )
                bOutputTraj = true;
            else if( cmd == "maxiter" )
                sinput >> params.nMaxIterations;
            else if( cmd == "visgraspthresh" )
                sinput >> params._fVisibiltyGraspThresh;
            else if( cmd == "execute" )
                sinput >> bExecute;
            else if( cmd == "writetraj" )
                sinput >> strtrajfilename;
            else if( cmd == "maxvelmult" )
                sinput >> fMaxVelMult;
            else if( cmd == "target" ) {
                string name; sinput >> name;
                ptarget = GetEnv()->GetKinBody(_stdmbstowcs(name.c_str()).c_str());
            }
            else if( cmd == "sensorindex" )
                sinput >> sensorindex;
            else if( cmd == "planner" )
                sinput >> plannername;
            else if( cmd == "convexfile" )
                sinput >> convexfilename;
            else if( cmd == "graspdistthresh" )
                sinput >> params._fGraspDistThresh;
            else if( stricmp(cmd.c_str(), "graspset") == 0 ) {
                params._vgrasps.clear(); params._vgrasps.reserve(200);
                string filename = getfilename_withseparator(sinput,';');
                ifstream fgrasp(filename.c_str());
                while(!fgrasp.eof()) {
                    TransformMatrix t;
                    fgrasp >> t;
                    if( !fgrasp ) {
                        break;
                    }
                    params._vgrasps.push_back(t);
                }
                RAVELOG_DEBUGA("grasp set size = %"PRIdS"\n", params._vgrasps.size());
            }
            else if( stricmp(cmd.c_str(), "gradientsamples") == 0 )
                sinput >> params._nGradientSamples;
            else break;

            if( !sinput ) {
                RAVELOG_ERRORA("failed\n");
                return false;
            }
        }

        if( ptarget == NULL ) {
            RAVELOG_ERRORA("no target specified\n");
            return false;
        }

        _probot->SetActiveDOFs(_probot->GetActiveManipulator()->_vecarmjoints);

        boost::shared_ptr<VisibilityConstraintFunction> pconstraint;
        if( convexfilename.size() > 0 ) {
            RAVELOG_DEBUGA("using visibility constraint with %s\n", convexfilename.c_str());
            try {
                pconstraint.reset(new VisibilityConstraintFunction(_probot,ptarget,convexfilename,sensorindex));
            }
            catch(const string& err) {
                RAVELOG_ERRORA(err.c_str());
                return false;
            }
        }

        params._ptarget = ptarget;
        params.pconstraintfn = pconstraint.get();
        _probot->GetActiveDOFValues(params.vinitialconfig);

        boost::shared_ptr<Trajectory> ptraj(GetEnv()->CreateTrajectory(_probot->GetActiveDOF()));
        boost::shared_ptr<PlannerBase> planner(GetEnv()->CreatePlanner(plannername.c_str()));
        if( planner.get() == NULL ) {
            RAVELOG_ERRORA("failed to create BiRRTs\n");
            return false;
        }
    
        bool bSuccess = false;
        RAVELOG_INFOA("starting planning\n");
        uint64_t starttime = GetMicroTime();
        if( !planner->InitPlan(_probot, &params) ) {
            RAVELOG_ERRORA("InitPlan failed\n");
            return false;
        }
        
        if( planner->PlanPath(ptraj.get()) ) {
            bSuccess = true;
        }
        else RAVELOG_WARNA("PlanPath failed\n");

        float felapsed = (GetMicroTime()-starttime)*0.000001f;
        RAVELOG_INFOA("total planning time: %fs\n", felapsed);
        if( !bSuccess )
            return false;

        SetTrajectory(ptraj.get(), bExecute, strtrajfilename, bOutputTraj?&sout:NULL,fMaxVelMult);
        sout << felapsed;
        return true;
    }

    bool SetTrajectory(Trajectory* pActiveTraj, bool bExecute, const string& strsavetraj, ostream* pout,dReal fMaxVelMult=1)
    {
        assert( pActiveTraj != NULL );
        if( pActiveTraj->GetPoints().size() == 0 )
            return false;

        pActiveTraj->CalcTrajTiming(_probot, pActiveTraj->GetInterpMethod(), true, true,fMaxVelMult);

        bool bExecuted = false;
        if( bExecute ) {
            if( pActiveTraj->GetPoints().size() > 1 ) {
                _probot->SetActiveMotion(pActiveTraj);
                bExecute = true;
            }
            // have to set anyway since calling script will orEnvWait!
            else if( _probot->GetController() != NULL ) {
                boost::shared_ptr<Trajectory> pfulltraj(GetEnv()->CreateTrajectory(_probot->GetDOF()));
                _probot->GetFullTrajectoryFromActive(pfulltraj.get(), pActiveTraj);

                if( _probot->GetController()->SetDesired(&pfulltraj->GetPoints()[0].q[0]))
                    bExecuted = true;
            }
        }

        if( strsavetraj.size() || pout != NULL ) {
            boost::shared_ptr<Trajectory> pfulltraj(GetEnv()->CreateTrajectory(_probot->GetDOF()));
            _probot->GetFullTrajectoryFromActive(pfulltraj.get(), pActiveTraj);

            if( strsavetraj.size() > 0 )
                pfulltraj->Write(strsavetraj.c_str(), Trajectory::TO_IncludeTimestamps|Trajectory::TO_IncludeBaseTransformation);

            if( pout != NULL )
                pfulltraj->Write(*pout, Trajectory::TO_IncludeTimestamps|Trajectory::TO_IncludeBaseTransformation|Trajectory::TO_OneLine);
        }
    
        return bExecuted;
    }

    bool AssemblyLine(ostream& sout, istream& sinput)
    {
        string cmd;
        while(!sinput.eof()) {
            sinput >> cmd;
            if( !sinput )
                break;
        
            if( cmd == "startpos" )
                sinput >> _abAssemblyStart.pos.x >> _abAssemblyStart.pos.y >> _abAssemblyStart.pos.z;
            else if( cmd == "startextents" )
                sinput >> _abAssemblyStart.extents.x >> _abAssemblyStart.extents.y >> _abAssemblyStart.extents.z;
            else if( cmd == "direction" )
                sinput >> _vAssemblyDirection.x >> _vAssemblyDirection.y >> _vAssemblyDirection.z;
            else if( cmd == "distance" )
                sinput >> _fAssemblyDistance;
            else if( cmd == "outputprob" )
                sinput >> _fOutputProbability;
            else if( cmd == "assobject" ) {
                ASSEMBLYOBJECT o;
                sinput >> o.filename >> o._fAppearProb;
                _listAssemblyTypes.push_back(o);
            }
            else if( cmd == "speed" )
                sinput >> _fAssemblySpeed;
            else break;

            if( !sinput ) {
                RAVELOG_ERRORA("failed\n");
                return false;
            }
        }

        return true;
    }

    bool SimulationStep(dReal fElapsedTime)
    {
        if( _fAssemblySpeed > 0 ) {

            if( _listAssemblyTypes.size() > 0 ) {
                if( GetEnv()->RandomFloat() < _fOutputProbability*fElapsedTime ) {
                    // attempt to place a collision free object
                    dReal totalprob = 0;
                    FOREACH(it,_listAssemblyTypes)
                        totalprob += it->_fAppearProb;

                    boost::shared_ptr<KinBody> pbody;
                    for(int iter = 0; iter < 10; ++iter) {
                        dReal objprob = totalprob*GetEnv()->RandomFloat();
                        ASSEMBLYOBJECT ao = _listAssemblyTypes.front();
                        FOREACH(ita, _listAssemblyTypes) {
                            if( objprob < ita->_fAppearProb ) {
                                ao = *ita;
                                break;
                            }
                            objprob -= ita->_fAppearProb;
                        }

                        pbody.reset(GetEnv()->ReadKinBodyXML(NULL, ao.filename.c_str(),NULL));
                        if( !!pbody )
                            break;
                    }
                
                    if( !pbody ) {
                        RAVELOG_WARNA("failed to create body in assembly line\n");
                        return false;
                    }

                    GetEnv()->AddKinBody(pbody.get());
                    bool bSuccess = false;

                    for(int iter = 0; iter < 100; ++iter) {
                        Transform t;
                        for(int j = 0; j < 3; ++j) {
                            t.trans[j] = _abAssemblyStart.pos[j] + (2.0f * GetEnv()->RandomFloat() - 1.0f)*_abAssemblyStart.extents[j];
                        }
                        pbody->SetTransform(t);
                        if( 1||!GetEnv()->CheckCollision(pbody.get()) ) {
                            bSuccess = true;
                            break;
                        }
                    }

                    if( bSuccess ) {
                        _listAssemblyObjects.push_back(pbody);
                        RAVELOG_DEBUGA("object created\n");
                    }
                    else
                        GetEnv()->RemoveKinBody(pbody.get(),false);
                }
            }

            // move the objects
            list<boost::shared_ptr<KinBody> >::iterator ito = _listAssemblyObjects.begin();
            while(ito != _listAssemblyObjects.end()) {
                Transform t = (*ito)->GetTransform();
                t.trans += _vAssemblyDirection * _fAssemblySpeed * fElapsedTime;
                (*ito)->SetTransform(t);
                if( dot3(_vAssemblyDirection,t.trans - _abAssemblyStart.pos) > _fAssemblyDistance ) {
                    GetEnv()->RemoveKinBody(ito->get(),false);
                    ito = _listAssemblyObjects.erase(ito);
                }
                else
                    ++ito;
            }
        }
        return false;
    }

    bool TestIK(ostream& sout, istream& sinput)
    {
        TransformMatrix tgrasp;
        string cmd;
        while(!sinput.eof()) {
            sinput >> cmd;
            if( !sinput )
                break;
        
            if( cmd == "matrix" )
                sinput >> tgrasp;
            if( !sinput ) {
                RAVELOG_ERRORA("failed\n");
                return false;
            }
        }
        

        return true;
    }

    bool Help(ostream& sout, istream& sinput)
    {
        sout << "------------------------" << endl
             << "VisualServoing Problem Commands:" << endl;
        GetCommandHelp(sout);
        sout << "------------------------" << endl;
        return true;
    }
    
protected:
    RobotBase* _probot;
    wstring _strRobotName;
    AABB _abAssemblyStart;
    Vector _vAssemblyDirection;
    dReal _fAssemblyDistance, _fOutputProbability, _fAssemblySpeed;
    list<ASSEMBLYOBJECT> _listAssemblyTypes;
    list<boost::shared_ptr<KinBody> > _listAssemblyObjects;
    
};

#endif