#buttons done
#todo for buttons.  
# Add attributes to Orientate2World vector for foot, pelvis, spine3 - maybe all spine? and add buttons for selected bodies?
# buttons added in C:\Program Files\NaturalMotion\endorphin_build4399_rev52936_incremental_win32_INTERNAL\scripts\Layout\StaticUI.lua

#loading as a module e.g.
#import ExportToEStudio as RS
#then using e.g. RS.changeEndorphinAttributes(physicsCharacter)
#leads to nmx not being recognised in RS.exportPhysicsCharacter 
#therefore just load in the script before the rest of this file
loadDefaultFile = False
execfile(r"C:\Program Files\NaturalMotion\endorphin_build4399_rev52936_incremental_win32_INTERNAL\scripts\RockstarRigging.py")
physicsCharacterPath = "|Scene|character|physicsCharacter"
physicsCharacter = nmx.scene().object(physicsCharacterPath)

axis = nmx.Vector3()
#orientate spine3 and pelvis to world as orientatePhysicsBodiesFromDefault:
# gets it wrong for the pelvis (incorrect parent and child joint)  
# gets it wrong for  spine3 (incorrect child joint).  Plus the spine3 is used to do directions in the code
#  so we want this to bea alligned to the world not spine3joint to neckjoint
bodyPath = "|Scene|character|physicsCharacter|SKEL_ROOT|SKEL_Pelvis|SKEL_PelvisPhysicsBodyTM|SKEL_PelvisPhysicsBody"
axis.set(0.0,0.0,-1.0)
pelvis = nmx.scene().object(bodyPath)
#------------------------------------
#Function 1
#------------------------------------
orientatePhysicsBodyFromDefaultBy2WorldAxis(pelvis,axis) 
transform = nmx.Transform(pelvis.parent())                            
Body_W = transform.WorldMatrix()                        
print "Pelvis position"
print Body_W.translation().x(),Body_W.translation().y(),Body_W.translation().z()

bodyPath = "|Scene|character|physicsCharacter|SKEL_ROOT|SKEL_Pelvis|SKEL_Spine0|SKEL_Spine1|SKEL_Spine2|SKEL_Spine3|SKEL_Spine3PhysicsBodyTM|SKEL_Spine3PhysicsBody"
axis.set(0.0,0.0,1.0)
spine3 = nmx.scene().object(bodyPath)
#------------------------------------
#Function 2
#------------------------------------
orientatePhysicsBodyFromDefaultBy2WorldAxis(spine3,axis) 

bodyPath = "|Scene|character|physicsCharacter|SKEL_ROOT|SKEL_Pelvis|SKEL_L_Thigh|SKEL_L_Calf|SKEL_L_Foot|SKEL_L_FootPhysicsBodyTM|SKEL_L_FootPhysicsBody"
foot = nmx.scene().object(bodyPath)
axis.set(0.0,1.0,0.0)
#------------------------------------
#Function 3
#------------------------------------
orientatePhysicsBodyFromDefaultBy2WorldAxis(foot,axis) 
#Check the world postions of the feet so the balancer doesn't lilt
transform = nmx.Transform(foot.parent())                            
Body_W = transform.WorldMatrix()                        
print "Left Foot Position"
print Body_W.translation().x(),Body_W.translation().y(),Body_W.translation().z()

bodyPath = "|Scene|character|physicsCharacter|SKEL_ROOT|SKEL_Pelvis|SKEL_R_Thigh|SKEL_R_Calf|SKEL_R_Foot|SKEL_R_FootPhysicsBodyTM|SKEL_R_FootPhysicsBody"
axis.set(0.0,1.0,0.0)
foot = nmx.scene().object(bodyPath)
#------------------------------------
#Function 4
#------------------------------------
orientatePhysicsBodyFromDefaultBy2WorldAxis(foot,axis) 
#Check the world postions of the feet so the balancer doesn't lilt
transform = nmx.Transform(foot.parent())
Body_W = transform.WorldMatrix()                        
print "Right Foot Position"
print Body_W.translation().x(),Body_W.translation().y(),Body_W.translation().z()















