#select physicsCharacter then press run
#character should be orientated x Down, y Forward, z Right

outputFilename = "C:\\endorphin3\\WorldSpaceXMLs\\ragdoll.xml"
changeNMJointValues = True
import math

#--------------------------------------------------------------------------------------------

def clamp(value, a, b):
  
  return min(max(value,a),b)

#--------------------------------------------------------------------------------------------

def acosSafe(value):
  
  return math.acos(clamp(value,-1.0,1.0))

#--------------------------------------------------------------------------------------------

def writeValueVector(outputFile, name, Xval, Yval, Zval, indent):
  
  format = "%.6f"
  outputFile.write(indent + '    <%s.X>%s</%s.X>  ' % (name, Xval, name))   
  outputFile.write('<%s.Y>%s</%s.Y>  ' % (name, Yval, name))   
  outputFile.write('<%s.Z>%s</%s.Z>\n' % (name, Zval, name))   
  
  #--------------------------------------------------------------------------------------------

def writeValueSingle(outputFile, name, value, indent):
  
  format = "%.6f"
  outputFile.write(indent + '  <%s>%s</%s>\n' % (name, value, name))

  
#--------------------------------------------------------------------------------------------

def matrixAsString(outputFile, indent, matrix):
  
  format = "%.6f"
  outputFile.write(indent + '    <xAxis_x>%s</xAxis_x>  ' % matrix.xAxis().x())   
  outputFile.write('<xAxis_y>%s</xAxis_y>  ' % matrix.xAxis().y())  
  outputFile.write('<xAxis_z>%s</xAxis_z>\n' % matrix.xAxis().z())  
  outputFile.write(indent + '    <yAxis_x>%s</yAxis_x>  ' % matrix.yAxis().x())   
  outputFile.write('<yAxis_y>%s</yAxis_y>  ' % matrix.yAxis().y())  
  outputFile.write('<yAxis_z>%s</yAxis_z>\n' % matrix.yAxis().z())  
  outputFile.write(indent + '    <zAxis_x>%s</zAxis_x>  ' % matrix.zAxis().x())   
  outputFile.write('<zAxis_y>%s</zAxis_y>  ' % matrix.zAxis().y())  
  outputFile.write('<zAxis_z>%s</zAxis_z>\n' % matrix.zAxis().z())  
  outputFile.write(indent + '    <trans_x>%s</trans_x>  ' % matrix.translation().x())   
  outputFile.write('<trans_y>%s</trans_y>  ' % matrix.translation().y())  
  outputFile.write('<trans_z>%s</trans_z>\n' % matrix.translation().z())  


#--------------------------------------------------------------------------------------------

def writePhysicsJointData(outputFile, physicsJoint, indent):

 

  # Get the local frame and world frame of the physics joint.

  physicsJoint_WorldFrame = physicsJoint.WorldMatrix()                        

  physicsJoint_LocalFrame = physicsJoint.LocalMatrix()  

  

  outputFile.write(indent + '<PhysicsJoint>\n')

  outputFile.write(indent + '  <name>%s</name>\n' % physicsJoint.Name)

  outputFile.write(indent + '  <path>%s</path>\n' % physicsJoint.path())

  outputFile.write(indent + '  <localFrame>\n')
  matrixAsString(outputFile, indent, physicsJoint_LocalFrame)  
  outputFile.write(indent + '  </localFrame>\n')
  #outputFile.write(indent + '  <localFrame>"%s"</localFrame>\n' % matrixAsString(physicsJoint_LocalFrame))    

  outputFile.write(indent + '  <worldFrame>\n')
  matrixAsString(outputFile, indent, physicsJoint_WorldFrame)  
  outputFile.write(indent + '  </worldFrame>\n')
  #outputFile.write(indent + '  <worldFrame>"%s"</worldFrame>\n' % matrixAsString(physicsJoint_WorldFrame))     

  outputFile.write(indent + '</PhysicsJoint>\n') 

 

#--------------------------------------------------------------------------------------------

def writePhysicsJointLimitData(outputFile, physicsJointLimit, indent):

 

  ## Get the local frame, world frame and world moving frame of the physics joint limit.  

  physicsJoint = nmx.PhysicsJointLimit(physicsJointLimit).physicsJoint()                            

  physicsJointLimit_WorldFrame = physicsJointLimit.WorldMatrix()

  physicsJointLimit_WorldMovingFrame = nmx.Matrix(nmx.Quaternion(nmx.Vector3(physicsJointLimit.MovingOffsetX(), physicsJointLimit.MovingOffsetY(), physicsJointLimit.MovingOffsetZ())))

  physicsJointLimit_WorldMovingFrame.multiply(physicsJoint.WorldMatrix())

  physicsJointLimit_LocalFrame = physicsJointLimit.LocalMatrix()
 
 

  outputFile.write(indent + '<PhysicsJointLimit>\n')

  outputFile.write(indent + '  <name>%s</name>\n' % physicsJointLimit.Name)

  outputFile.write(indent + '  <path>%s</path>\n' % physicsJointLimit.path())

  outputFile.write(indent + '  <type>%s</type>\n' % physicsJointLimit.LimitType)

  

  # limit ranges

  if physicsJointLimit.LimitType.string() == "Hinge":

    outputFile.write(indent + '  <twistAngle>%s</twistAngle>\n' % physicsJointLimit.TwistAngle)

    outputFile.write(indent + '  <twistOffset>%s</twistOffset>\n' % physicsJointLimit.TwistOffset)
    
    # Copied from the to-Euphoria exporter
    
    writeValueVector(outputFile, 'axisposition', physicsJointLimit_WorldFrame.translation().x(), physicsJointLimit_WorldFrame.translation().y(), physicsJointLimit_WorldFrame.translation().z(), indent)
    writeValueVector(outputFile, 'axisdirection', physicsJointLimit_WorldFrame.xAxis().x(), physicsJointLimit_WorldFrame.xAxis().y(), physicsJointLimit_WorldFrame.xAxis().z(), indent)
    writeValueSingle(outputFile, 'limitenabled', physicsJointLimit.Limitenabled.boolean(), indent)        
    #NB. Rotating the joint limit around the hinge axis has no effect (in endorphin either)
    #moving frame can be outside of limit offset - mmmmtodo WARN here as why would you want to do that?
    max = 0.5*physicsJointLimit.TwistAngle.double() + physicsJointLimit.TwistOffset.double()
    min = physicsJointLimit.TwistOffset.double() - 0.5*physicsJointLimit.TwistAngle.double()
    writeValueSingle(outputFile, 'minangle', min, indent)
    writeValueSingle(outputFile, 'maxangle', max, indent)
    writeValueSingle(outputFile, 'createeffector', physicsJointLimit.Createeffector.boolean(), indent)
    writeValueSingle(outputFile, 'defaultleanforcecap', physicsJointLimit.Defaultleanforcecap.double(), indent)
    writeValueSingle(outputFile, 'defaultmusclestiffness', physicsJointLimit.Defaultmusclestiffness.double(), indent)
    writeValueSingle(outputFile, 'defaultmusclestrength', physicsJointLimit.Defaultmusclestrength.double(), indent)
    writeValueSingle(outputFile, 'defaultmuscledamping', physicsJointLimit.Defaultmuscledamping.double(), indent)    
    if changeNMJointValues:
      physicsJointLimit.Minangle.setFloat(min)
      physicsJointLimit.Maxangle.setFloat(max)
    
  elif physicsJointLimit.LimitType.string() == "BallSocket":

    outputFile.write(indent + '  <swing1Angle>%s</swing1Angle>\n' % physicsJointLimit.Swing1Angle)

    outputFile.write(indent + '  <swing2Angle>%s</swing2Angle>\n' % physicsJointLimit.Swing2Angle)

    outputFile.write(indent + '  <twistAngle>%s</twistAngle>\n' % physicsJointLimit.TwistAngle)

    outputFile.write(indent + '  <twistOffset>%s</twistOffset>\n' % physicsJointLimit.TwistOffset)
    
    # Copied from the to-Euphoria exporter
     
    #axis direction and leandirection define the plane that lean1 moves the body in.
    writeValueVector(outputFile, 'axisposition', physicsJointLimit_WorldFrame.translation().x(), physicsJointLimit_WorldFrame.translation().y(), physicsJointLimit_WorldFrame.translation().z(), indent)
    writeValueVector(outputFile, 'axisdirection', physicsJointLimit_WorldMovingFrame.xAxis().x(), physicsJointLimit_WorldMovingFrame.xAxis().y(), physicsJointLimit_WorldMovingFrame.xAxis().z(), indent)
    writeValueSingle(outputFile, 'limitenabled', physicsJointLimit.Limitenabled.boolean(), indent)
    writeValueSingle(outputFile, 'createeffector', physicsJointLimit.Createeffector.boolean(), indent)
    writeValueVector(outputFile, 'leandirection', physicsJointLimit_WorldMovingFrame.zAxis().x(), physicsJointLimit_WorldMovingFrame.zAxis().y(), physicsJointLimit_WorldMovingFrame.zAxis().z(), indent)
    halfSwing1 = physicsJointLimit.Swing1Angle.double()
    halfSwing2 = physicsJointLimit.Swing2Angle.double()
    lean2Offset = math.copysign(1, physicsJointLimit_WorldMovingFrame.xAxis().dotProduct(physicsJointLimit_WorldFrame.yAxis())) * acosSafe(physicsJointLimit_WorldMovingFrame.yAxis().dotProduct(physicsJointLimit_WorldFrame.yAxis()))
    rotatedWorlFrame = nmx.Matrix()
    rotatedWorlFrame.set(physicsJointLimit_WorldFrame)
    #Gave a slight error for lean1min/max if lean1 and lean2 limits asymmetric e.g shoulder, hip
    #rotatedWorlFrame.rotateByAxisAngle(physicsJointLimit_WorldMovingFrame.zAxis(),lean2Offset)
    rotatedWorlFrame.rotateByAxisAngle(rotatedWorlFrame.zAxis(),lean2Offset)
 
    lean1Offset = math.copysign(1, physicsJointLimit_WorldMovingFrame.zAxis().dotProduct(rotatedWorlFrame.xAxis())) * acosSafe(physicsJointLimit_WorldMovingFrame.xAxis().dotProduct(rotatedWorlFrame.xAxis()))
    #print "angle, offset"
    #print halfSwing1, lean1Offset
    #print halfSwing2, lean2Offset
    writeValueSingle(outputFile, 'minfirstleanangle', -halfSwing1 + lean1Offset, indent)
    writeValueSingle(outputFile, 'maxfirstleanangle', halfSwing1 + lean1Offset, indent)
    writeValueSingle(outputFile, 'minsecondleanangle', -halfSwing2 + lean2Offset, indent)
    writeValueSingle(outputFile, 'maxsecondleanangle', halfSwing2 + lean2Offset, indent)
    min = -0.5*physicsJointLimit.TwistAngle.double() - physicsJointLimit.TwistOffset.double()
    max =  0.5*physicsJointLimit.TwistAngle.double() - physicsJointLimit.TwistOffset.double()
    writeValueSingle(outputFile, 'mintwistangle', min, indent)
    writeValueSingle(outputFile, 'maxtwistangle', max, indent)
    writeValueSingle(outputFile, 'reversefirstleanmotor', physicsJointLimit.Reversefirstleanmotor.boolean(), indent)
    writeValueSingle(outputFile, 'reversesecondleanmotor', physicsJointLimit.Reversesecondleanmotor.boolean(), indent)
    writeValueSingle(outputFile, 'reversetwistmotor', physicsJointLimit.Reversetwistmotor.boolean(), indent)
    writeValueSingle(outputFile, 'softlimitfirstleanmultiplier', physicsJointLimit.Softlimitfirstleanmultiplier.double(), indent)
    writeValueSingle(outputFile, 'softlimitsecondleanmultiplier', physicsJointLimit.Softlimitsecondleanmultiplier.double(), indent)
    writeValueSingle(outputFile, 'softlimittwistmultiplier', physicsJointLimit.Softlimittwistmultiplier.double(), indent)
    writeValueSingle(outputFile, 'defaultleanforcecap', physicsJointLimit.Defaultleanforcecap.double(), indent)
    writeValueSingle(outputFile, 'defaulttwistforcecap', physicsJointLimit.Defaulttwistforcecap.double(), indent)
    writeValueSingle(outputFile, 'defaultmusclestiffness', physicsJointLimit.Defaultmusclestiffness.double(), indent)
    writeValueSingle(outputFile, 'defaultmusclestrength', physicsJointLimit.Defaultmusclestrength.double(), indent)
    writeValueSingle(outputFile, 'defaultmuscledamping', physicsJointLimit.Defaultmuscledamping.double(), indent)

    if changeNMJointValues:
      physicsJointLimit.Minfirstleanangle.setFloat(-halfSwing1 + lean1Offset)
      physicsJointLimit.Maxfirstleanangle.setFloat(halfSwing1 + lean1Offset)
      physicsJointLimit.Minsecondleanangle.setFloat(-halfSwing2 + lean2Offset)
      physicsJointLimit.Maxsecondleanangle.setFloat(halfSwing2 + lean2Offset)
      physicsJointLimit.Mintwistangle.setFloat(min)
      physicsJointLimit.Maxtwistangle.setFloat(max)


  #outputFile.write(indent + '   <localFrame>"%s"</localFrame>\n' % matrixAsString(physicsJointLimit_LocalFrame))
  outputFile.write(indent + '  <localFrame>\n')
  matrixAsString(outputFile, indent, physicsJointLimit_LocalFrame)
  outputFile.write(indent + '  </localFrame>\n')

  #outputFile.write(indent + '   <worldFixedFrame>"%s"</worldFixedFrame>\n' % matrixAsString(physicsJointLimit_WorldFrame))    
  outputFile.write(indent + '  <worldFixedFrame>\n')
  matrixAsString(outputFile, indent, physicsJointLimit_WorldFrame) 
  outputFile.write(indent + '  </worldFixedFrame>\n')

  #outputFile.write(indent + '   <worldMovingFrame>"%s"</worldMovingFrame>\n' % matrixAsString(physicsJointLimit_WorldMovingFrame))    
  outputFile.write(indent + '  <worldMovingFrame>\n')
  matrixAsString(outputFile, indent, physicsJointLimit_WorldMovingFrame)
  outputFile.write(indent + '  </worldMovingFrame>\n')

  outputFile.write(indent + '</PhysicsJointLimit>\n') 

 

#--------------------------------------------------------------------------------------------

def writePhysicsBodyData(outputFile, physicsBody, indent):

 

  # Get the local frame and world frame of the transform that positions the physics body.

  transform = nmx.Transform(physicsBody.parent())                            

  physicsBody_WorldFrame = transform.WorldMatrix()                        

  physicsBody_LocalFrame = transform.LocalMatrix()    

 

  outputFile.write(indent + '<PhysicsBody>\n')

  outputFile.write(indent + '  <name>%s</name>\n' % physicsBody.Name)
  print physicsBody.Name
  outputFile.write(indent + '  <path>%s</path>\n' % physicsBody.path())

  outputFile.write(indent + '  <shape>%s</shape>\n' % physicsBody.Shape)


  # dimensions depending on shape type

  if physicsBody.Shape.string() == "Sphere":

    outputFile.write(indent + '  <radius>%s</radius>\n' % physicsBody.Radius)

  elif physicsBody.Shape.string() == "Capsule":

    outputFile.write(indent + '  <length>%s</length>\n' % physicsBody.Length)

    outputFile.write(indent + '  <radius>%s</radius>\n' % physicsBody.Radius)

  elif physicsBody.Shape.string() == "Box":

    outputFile.write(indent + '  <height>%s</height>\n' % physicsBody.Height)

    outputFile.write(indent + '  <length>%s</length>\n' % physicsBody.Length)
 
    outputFile.write(indent + '  <width>%s</width>\n' % physicsBody.Width)

 

  outputFile.write(indent + '  <mass>%s</mass>\n' % physicsBody.Rs_mass)
  

  outputFile.write(indent + '  <localFrame>\n')
  matrixAsString(outputFile, indent, physicsBody_LocalFrame)
  outputFile.write(indent + '  </localFrame>\n') 
  
  outputFile.write(indent + '  <worldFrame>\n')
  matrixAsString(outputFile, indent, physicsBody_WorldFrame)
  outputFile.write(indent + '  </worldFrame>\n')  
  
  outputFile.write(indent + '</PhysicsBody>\n') 

 

#--------------------------------------------------------------------------------------------

def recursiveWritePhysicsCharacterData(outputFile, object, indent):

    

  childObjects = object.childObjects()

  if len(childObjects) > 0:

    outputFile.write(indent + '  <ChildObjects>\n')

    newIndent = indent + '    '

    for childObject in childObjects:

      if childObject.isType('PhysicsBody'):

        writePhysicsBodyData(outputFile, nmx.PhysicsBody(nmx.PhysicsBody(childObject)), newIndent)

      elif childObject.isType('PhysicsJoint'):

        writePhysicsJointData(outputFile, nmx.PhysicsJoint(nmx.PhysicsJoint(childObject)), newIndent)

      elif childObject.isType('PhysicsJointLimit'):

        writePhysicsJointLimitData(outputFile, nmx.PhysicsJointLimit(nmx.PhysicsJointLimit(childObject)), newIndent)

      recursiveWritePhysicsCharacterData(outputFile, childObject, newIndent)

    outputFile.write(indent + '  </ChildObjects>\n')



#--------------------------------------------------------------------------------------------

def writeCollisionSets(outputFile, indent):

  newIndent = indent + '  '

  outputFile.write(newIndent + '<CollisionSets>\n')

  allCollisionSets = nmx.ObjectArray()

  nmx.ObjectGroup(nmx.scene().CollisionGroups).childGroups(allCollisionSets)

  

  for i in allCollisionSets:

    outputFile.write(newIndent + '  <CollisionSet>\n')
    
    outputFile.write(newIndent + '    <name>%s</name>\n' % i.Label)

    group = nmx.CollisionGroup(i)

    physicsJoints = nmx.ObjectArray()

    group.physicsJoints(physicsJoints)

    for joint in physicsJoints:

      body = nmx.PhysicsJoint(joint).physicsBody()

      outputFile.write(newIndent + '    <item>%s</item>\n' % body.Name)    

      #outputFile.write(newIndent + '    <item>%s</item>\n' % joint.Name)    

    outputFile.write(newIndent + '  </CollisionSet>\n')

 

  outputFile.write(newIndent + '</CollisionSets>\n')

  

#--------------------------------------------------------------------------------------------

def writePhysicsCharacterData(outputFile, physicsCharacter):

  outputFile.write('<PhysicsCharacter>\n')  

  indent = ''

  recursiveWritePhysicsCharacterData(outputFile, physicsCharacter, indent)  

  writeCollisionSets(outputFile, indent)

  outputFile.write('</PhysicsCharacter>\n')

#--------------------------------------------------------------------------------------------

# Recursively walk down the physics character hierarchies.

# Otherwise write them out to a file

outputFile = open(outputFilename, "w")

# only export selected node ...

if nmx.scene().Selection.hasSelection() and (nmx.scene().Selection.object(0).Name.string() == "physicsCharacter"):  

  writePhysicsCharacterData(outputFile, nmx.scene().Selection.object(0));

# ... or all physics rigs  

else:

  physicsCharacters = nmx.scene().objectsExact("PhysicsCharacter")

  for physicsCharacter in physicsCharacters:

    writePhysicsCharacterData(outputFile,physicsCharacter);

outputFile.close()

#--------------------------------------------------------------------------------------------



















