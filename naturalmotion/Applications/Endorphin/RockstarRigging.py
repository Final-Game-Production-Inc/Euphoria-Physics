import struct
import math
import os.path
from nmx import *
#import nmx

#############################################################################################
## Common Functions and Dictionaries 
#############################################################################################
Pi = 4.0*math.atan(1.0)
DegToRad = Pi/180.0
RadToDeg = 1.0/DegToRad
Xaxis = 1
Yaxis = 2
Zaxis = 3
volumeIDPrefix = 100000000000
shapeIDPrefix = 200000000000
articulatedPartIDPrefix = 300000000000
jointIDPrefix = 400000000000
collisionSetIDPrefix = 500000000000
jointNumber = 0
rootPos = nmx.Vector3()
double2Hex = False

def endorphinToRockstarName(Name):
  "Converts the endorphin name for objects to R* names for joints and bodies \n"
  Name = str(Name)
  try: 
    rsName = objectName[Name]
  except KeyError:
   rsName = "ERROR_UnknownEndorphinName"
   print "ERROR: Unknown Endorphin Name: " + Name

  return str(rsName)

#--------------------------------------------------------------------------------------------
def acosSafe(value):
  "Finds the arccosine of a number and doesn't fail if abs(value) > 1"
  if (value < 1.0 and value > -1.0):
    return math.acos(value)
  elif (value >= 1.0):
    return 0.0
  else:
    return 3.14159

#--------------------------------------------------------------------------------------------
# Don't use - Works 99% of the time - fails for some numbers though
def doubleTohexPython(value):
  "Writes an ieee754 floating point in ascii hex"
  double = struct.pack('>d', value)
  left, right = double[4:], double[:4]
  return ''.join([hex(ord(c))[2:].ljust(2, '0') for c in ''.join((left, right))])
  #return '%f' % value #Uncomment this line and comment out above to output a float in float format

#--------------------------------------------------------------------------------------------
# Used to create old art files (121.xml). Is an example of accessing a dll
def doubleTohex(value):
  "Writes an ieee754 floating point in ascii hex\n"\
  " using the art c function"
  # Import the ctypes module
  ctypes = __import__("ctypes")
  # Load our own dll
  custom = ctypes.cdll.LoadLibrary(r"D:\NM\assets\euphoriaStudioProduction\Rockstar\Character\Endorphin\ExportUtils\debug\ExportUtils")
  # Create a mutable string buffer to write to
  buffer = ctypes.create_string_buffer(64)
  # Call our hex conversion function
  custom.doubleTohex(ctypes.c_double(value), buffer)
  # Return our hex string
  return str(buffer.value)

#--------------------------------------------------------------------------------------------
def writeFloat(value):
  "Writes an ieee754 floating point number\n"
  global double2Hex
  if double2Hex:
    return doubleTohex(value)
  else:
    return '%.9g' % value

#--------------------------------------------------------------------------------------------
def convertCapsuleMatrixToRS(matrix):
  "Convert the endorphin capsule matrix to that used in R* eStudio\n"\
  "(Capsules are Z down length in endorphin but y up length in eStudio)"
  matrix_loc = nmx.Matrix()#don't change the function argument 'matrix'
  matrix_loc.set(matrix)
  matrixT = nmx.Matrix()
  matrixT.set(nmx.Vector3(-1.0,0.0,0.0), nmx.Vector3(0.0,0.0,-1.0), nmx.Vector3(0.0,-1.0,0.0), nmx.Vector3(0.0,0.0,0.0))
  matrixOut = nmx.Matrix()
  matrixOut.multiply(matrix_loc.transpose3x3(), matrixT)
  matrixOut.setTranslation(matrix_loc.translation())
  return matrixOut


#############################################################################################
## Functions and Dictionaries for Adding RS attributes 
##  or changing existing endorphin attributes (that are RS attributes) 
#############################################################################################
#--------------------------------------------------------------------------------------------
#endorphin enumerations
#--------------------------------------------------------------------------------------------
#Physics Body: Shape
enum = {"Box" : 0}
enum["Capsule"] = 1
enum["Cylinder"] = 2
enum["Sphere"] = 3
#Physics Joint Limit: Limit Type 
enum["BallSocket"] = 0
enum["Hinge"] = 1

#--------------------------------------------------------------------------------------------
#Ranges for added RS attributes
#--------------------------------------------------------------------------------------------
#3dofs
range = {"Minfirstleanangle" : (-2*Pi, 2*Pi)}
range["Maxfirstleanangle"] = (-2*Pi, 2*Pi)  
range["Minfirstleanangle"] = (-2*Pi, 2*Pi)  
range["Maxfirstleanangle"] = (-2*Pi, 2*Pi)  
range["Minsecondleanangle"] = (-2*Pi, 2*Pi)  
range["Maxsecondleanangle"] = (-2*Pi, 2*Pi)  
range["Mintwistangle"] = (-2*Pi, 2*Pi)  
range["Maxtwistangle"] = (-2*Pi, 2*Pi)  
range["Softlimitfirstleanmultiplier"] = (0, 10)  
range["Softlimitsecondleanmultiplier"] = (0, 10)  
range["Softlimittwistmultiplier"] = (0, 10)  
range["Defaultleanforcecap"] = (0, 10000)  
range["Defaulttwistforcecap"] = (0, 10000)  
range["Defaultmusclestiffness"] = (0.0001, 100)  
range["Defaultmusclestrength"] = (0.0001, 10000)  
range["Defaultmuscledamping"] = (0.0001, 1000)  
#1dofs
range["Minangle"] = (-2*Pi, 2*Pi)  
range["Maxangle"] = (-2*Pi, 2*Pi)  
#bodies
range["CentreofmassX"] = (-100, 100)  
range["CentreofmassY"] = (-100, 100)  
range["CentreofmassZ"] = (-100, 100)  
range["Rs_mass"] = (0, 100)  
range["Lineardamping"] = (0.00001, 10)  
range["Angulardamping"] = (0.00001, 10)  
range["Gravityscale"] = (0, 100)  
range["Inertiamultiplier"] = (0.00001, 100)  
#all
range["BoneAxisIndex"] = (-3, 3)  
range["AlignAxisIndex"] = (-3, 3)  
range["WorldAlignAxisIndex"] = (-3, 3)  

#--------------------------------------------------------------------------------------------
#default values for added RS attributes
#--------------------------------------------------------------------------------------------
#default values for unknown rockstar body or joint
default = {"ERROR_UnknownEndorphinName" : {
  #3dofs
   "Limitenabled" : True
  ,"Minfirstleanangle" : -0.5
  ,"Maxfirstleanangle" : 0.5
  ,"Minsecondleanangle" : -0.5
  ,"Maxsecondleanangle" : 0.5
  ,"Mintwistangle" : -0.5
  ,"Maxtwistangle" : 0.5
  ,"Createeffector" : True    
  ,"Reversefirstleanmotor" : False
  #if it is a right limb then reverse the lean2 and twist motors
  ,"Reversesecondleanmotor" : False
  ,"Reversetwistmotor" : False
  ,"Softlimitfirstleanmultiplier" : 1
  ,"Softlimitsecondleanmultiplier" : 1
  ,"Softlimittwistmultiplier" : 1
  ,"Defaultleanforcecap" : 300
  ,"Defaulttwistforcecap" : 300
  ,"Defaultmusclestiffness" : 0.5
  ,"Defaultmusclestrength" : 100
  ,"Defaultmuscledamping" : 20
  #1dofs
  ,"Minangle" : -2.44346
  ,"Maxangle" : 0
  ,"CreateEffector" : True   
  ,"Defaultleanforcecap" : 1000,
  #bodies
  "BoneAxisIndex" : Zaxis,
  "AlignAxisIndex" : Yaxis,
  "WorldAlignAxisIndex" : Yaxis,
  "Hascollision" : True,
  "CentreofmassX" :  0, 
  "CentreofmassY" :  0,
  "CentreofmassZ" :  0, 
  "Rs_mass" : 13.8, 
  "Lineardamping" : 0.1, 
  "Angulardamping" : 0.1, 
  "Gravityscale" : 1,
  "Inertiamultiplier" : 0.00001}}
#endorphin body defaults 
endorphinDefault = {"HackJustToInitializeDictionary" : {
  "StaticFriction" : 0.75,
  "Restitution" :  0.4, 
  "Shape" : "Capsule",
  "Length" :  0.125, 
  "Radius" :  0.133}}

#load the character defaults
if loadDefaultFile == True:
  fileName = physicsCharacter.OutputDirectory.string() + physicsCharacter.AssetName.string() + "_Defaults.py"
  print "Loading defaults file %s" % fileName
  execfile(fileName)
  fileName = physicsCharacter.OutputDirectory.string() + physicsCharacter.AssetName.string() + "_Endorphin2RockstarNames.py"
  print "Loading Name Map file %s" % fileName
  execfile(fileName)
  


def addDefaultFloat(object, attributeName):
  "Add a float attribute to the endorphin object\n"\
  "Will appear in the objects attribute editor in the Custom Attributes section"
  #output default value if not a known rockstar name for a body or joint
  #  warning will have been output by endorphinToRockstarName function
  rockstarName = endorphinToRockstarName(object.Name)
  min = 0
  max = 1
  defaultValue = 0 
  try:
    defaultValues = default[rockstarName]
  except KeyError:
    print("ERROR: default." + rockstarName + " not set")
    print("  Writing defaultValue = 0, min = 0, max = 0")
  else:
    try:
      defaultValueTry = default[rockstarName][attributeName]
    except KeyError:
      print("ERROR: Unknown attribute: " + attributeName + " Or default." + rockstarName + "." + attributeName + " not set")
      print("  Writing defaultValue = 0, min = 0, max = 0")
    else:
      defaultValue = default[rockstarName][attributeName]
      try:
        rangeVar = range[attributeName]
      except KeyError:
        print("ERROR: Unknown attribute: " + attributeName + " Or range." + attributeName + " min/max not set")
        print("  Writing defaultValue = 0, min = 0, max = 0")
      else:
        min = range[attributeName][0]
        max = range[attributeName][1]
  
  object.addFloatAttribute(attributeName, defaultValue, min, max, False)

def addDefaultInt(object, attributeName):
  "Add an integer attribute to the endorphin object\n"\
  "Will appear in the objects attribute editor in the Custom Attributes section"
  #output default value if not a known rockstar name for a body or joint
  #  warning will have been output by endorphinToRockstarName function
  rockstarName = endorphinToRockstarName(object.Name)
  min = 0
  max = 1
  defaultValue = 0 
  try:
    defaultValues = default[rockstarName]
  except KeyError:
    print("ERROR: default." + rockstarName + " not set")
    print("  Writing defaultValue = 0, min = 0, max = 0")
  else:
    try:
      defaultValueTry = default[rockstarName][attributeName]
    except KeyError:
      print("ERROR: Unknown attribute: " + attributeName + " Or default." + rockstarName + "." + attributeName + " not set")
      print("  Writing defaultValue = 0, min = 0, max = 0")
    else:
      defaultValue = default[rockstarName][attributeName]
      try:
        rangeVar = range[attributeName]
      except KeyError:
        print("ERROR: Unknown attribute: " + attributeName + " Or range." + attributeName + " min/max not set")
        print("  Writing defaultValue = 0, min = 0, max = 0")
      else:
        min = range[attributeName][0]
        max = range[attributeName][1]
  
  object.addIntegerAttribute(attributeName, defaultValue, min, max, False)

def addDefaultBool(object, attributeName):
  "Add a boolean attribute to the endorphin object\n"\
  "Will appear in the objects attribute editor in the Custom Attributes section"
  #output default value if not a known rockstar name for a body or joint
  #  warning will have been output by endorphinToRockstarName function
  rockstarName = endorphinToRockstarName(object.Name)  
  defaultValue = False
  try:
    defaultValues = default[rockstarName]
  except KeyError:
    print("ERROR: default." + rockstarName + " not set")
    print("  Writing defaultValue = False")
  else:
    try:
      defaultValueTry = default[rockstarName][attributeName]
    except KeyError:
      print("ERROR: Unknown attribute: " + attributeName + " Or default." + rockstarName + "." + attributeName + " not set")
      print("  Writing defaultValue = False")
    else:
      defaultValue = default[rockstarName][attributeName]

  object.addBooleanAttribute(attributeName,defaultValue, False)
  print(attributeName,defaultValue)

def addJointLimitAttributes(jointLimit): 
  "Adds Rockstar joint attributes to an endorphin PhysicsJointLimit object\n"\
  "These will appear in the PhysicsJointLimit's attribute editor in the Custom Attributes section"
  # Loop through them printing there names
  print("Physics Joint Limit")
  #Common rockstar joint attributes
  addDefaultFloat(jointLimit,"Defaultmusclestiffness")
  addDefaultFloat(jointLimit, "Defaultmusclestrength")
  addDefaultFloat(jointLimit, "Defaultmuscledamping")
  addDefaultBool(jointLimit, "Limitenabled")
  addDefaultBool(jointLimit, "Createeffector")
  addDefaultInt(jointLimit, "BoneAxisIndex")
  addDefaultInt(jointLimit, "AlignAxisIndex")
  addDefaultInt(jointLimit, "WorldAlignAxisIndex")
  if jointLimit.LimitType.string() == "BallSocket":
    print("3dof")
    addDefaultFloat(jointLimit, "Minfirstleanangle")
    addDefaultFloat(jointLimit, "Maxfirstleanangle")
    addDefaultFloat(jointLimit, "Minsecondleanangle")
    addDefaultFloat(jointLimit, "Maxsecondleanangle")
    addDefaultFloat(jointLimit, "Mintwistangle")
    addDefaultFloat(jointLimit, "Maxtwistangle")
    addDefaultBool(jointLimit, "Reversefirstleanmotor")
    addDefaultBool(jointLimit, "Reversesecondleanmotor")
    addDefaultBool(jointLimit, "Reversetwistmotor")
    addDefaultFloat(jointLimit, "Softlimitfirstleanmultiplier")
    addDefaultFloat(jointLimit, "Softlimitsecondleanmultiplier")
    addDefaultFloat(jointLimit, "Softlimittwistmultiplier")
    addDefaultFloat(jointLimit, "Defaultleanforcecap")
    addDefaultFloat(jointLimit, "Defaulttwistforcecap")
  elif jointLimit.LimitType.string() == "Hinge":
    print("1dof")
    addDefaultFloat(jointLimit, "Minangle")
    addDefaultFloat(jointLimit, "Maxangle")
    addDefaultFloat(jointLimit, "Defaultleanforcecap")
  else:
    print ("WARNING: unknown joint type for " +  jointLimit.Name)

def addBodyAttributes(body):
  "Adds Rockstar body attributes to an endorphin PhysicsBody object\n"\
  "These will appear in the PhysicsBody's attribute editor in the Custom Attributes section"
  ## Loop through them printing their names
  print("Physics Body")
  ##Common rockstar body attributes
  addDefaultBool(body, "Hascollision")
  addDefaultFloat(body, "CentreofmassX")
  addDefaultFloat(body, "CentreofmassY")
  addDefaultFloat(body, "CentreofmassZ")
  addDefaultFloat(body, "Rs_mass")
  addDefaultFloat(body, "Lineardamping")
  addDefaultFloat(body, "Angulardamping")
  addDefaultFloat(body, "Gravityscale")
  addDefaultFloat(body, "Inertiamultiplier")
  addDefaultInt(body, "BoneAxisIndex")
  addDefaultInt(body, "AlignAxisIndex")
  addDefaultInt(body, "WorldAlignAxisIndex")

def changeEndorphinFloat(object, attributeName):
  "Change a float attribute in the endorphin object"
  #output default value if not a known rockstar name for a body or joint
  #  warning will have been output by endorphinToRockstarName function
  rockstarName = endorphinToRockstarName(object.Name)
  defaultValue = "ERROR"
  try:
    defaultValue = endorphinDefault[rockstarName]
  except KeyError:
    #don't change the value
    print("ERROR: endorphinDefault." + rockstarName + " not set")
  else:
    try:
      defaultValueTry = endorphinDefault[rockstarName][attributeName]
    except KeyError:
      #don't change the value
      print("ERROR: Unknown attribute: " + attributeName + " Or endorphinDefault." + rockstarName + "." + attributeName + " not set")
    else:
      defaultValue = endorphinDefault[rockstarName][attributeName]
      attribute = object.attribute(attributeName)
      print(rockstarName, attributeName, defaultValue, type(defaultValue))
      #print(attributeName + " = %f" % defaultValue)
      attribute.setFloat(defaultValue)
  return defaultValue

def changeEndorphinString(object, attributeName):
  "Change a string attribute in the endorphin object\n"\
  "Actually this inserts an enumeration index of a known list\n"\
  "stored in the enum dictionary in this file"\
  #output default value if not a known rockstar name for a body or joint
  #  warning will have been output by endorphinToRockstarName function
  #in lua you could set the string directly.  Python needs an enumeration of the list
  rockstarName = endorphinToRockstarName(object.Name)
  defaultValueString = "ERROR"
  try:
    defaultValueString = endorphinDefault[rockstarName]
  except KeyError:
    #don't change the value
    print("ERROR: endorphinDefault." + rockstarName + " not set")
  else:
    try:
      defaultValueTry = endorphinDefault[rockstarName][attributeName]
    except KeyError:
      #don't change the value
      print("ERROR: Unknown attribute: " + attributeName + " Or endorphinDefault." + rockstarName + "." + attributeName + " not set")
    else:
      defaultValueStringTry = endorphinDefault[rockstarName][attributeName]
      try:
        defaultValueTry = enum[defaultValueStringTry]
      except KeyError:
        #don't change the value
        print("ERROR: Unknown enum for: " + defaultValueString)
      else:
        defaultValue = defaultValueTry
        defaultValueString = defaultValueStringTry
        attribute = object.attribute(attributeName)
        attribute.setEnumeration(defaultValue)
  return defaultValueString

def changeEndorphinBodyAttributes(body):
  # Loop through them printing there names
  print("Physics Body")
  #Common rockstar joint attributes
  shape = changeEndorphinString(body, "Shape")
  changeEndorphinFloat(body, "StaticFriction")
  changeEndorphinFloat(body, "Restitution")
  if shape == "Box":
    changeEndorphinFloat(body, "Height")
    changeEndorphinFloat(body, "Length")
    changeEndorphinFloat(body, "Width")
  elif shape == "Capsule":
    changeEndorphinFloat(body, "Radius")
    #change the capsule length?
    #if (nmx.scene().object("|Scene|character|physicsCharacter").attribute("ChangeCapsuleLength").boolean()):
    changeEndorphinFloat(body, "Length")
  else:
    print ("ERROR: unknown shape: " + shape + " for body: " + body.Name.string())

def changeEndorphinJointLimitAttributes(joint):
  # Loop through them printing there names
  print("Physics Joint")
  #Common rockstar joint attributes
  jointType = changeEndorphinString(joint, "LimitType") 
  if jointType != "ERROR":
    if jointType == "BallSocket":
      changeEndorphinFloat(joint, "Swing1Angle")
      changeEndorphinFloat(joint, "Swing2Angle")
      changeEndorphinFloat(joint, "TwistAngle")
      changeEndorphinFloat(joint, "TwistOffset")
    elif jointType == "Hinge":
      changeEndorphinFloat(joint, "TwistAngle")
      changeEndorphinFloat(joint, "TwistOffset")
    else:
      print ("ERROR: unknown jointType: " + jointType + " for body: " + joint.Name)
  else:
    print ("ERROR: unknown joint: " + joint.Name.string())

def addRockstarAttributes(object):
  "Adds Rockstar joint/body attributes to endorphin PhysicsJointLimits/PhysicsBodies \n"\
  "These will appear in the PhysicsJointLimit's/PhysicsBody's attribute editor in the Custom Attributes section"
  addRockstarAttributesToBodiesAndJoints(object)
  
  
def addRockstarAttributesToBodiesAndJoints(object): 
  "Adds Rockstar joint/body attributes to endorphin PhysicsJointLimits/PhysicsBodies \n"\
  "These will appear in the PhysicsJointLimit's/PhysicsBody's attribute editor in the Custom Attributes section"
  # Recurse through the tree
  childObjects = object.childObjects()
  for child in childObjects:
    print(child.Name)
    if child.isType("PhysicsJointLimit"):
      addJointLimitAttributes(child)
    elif child.isType("PhysicsBody"):
      addBodyAttributes(child)
    addRockstarAttributesToBodiesAndJoints(child)


def changeEndorphinAttributes(object): 
  "Changes endorphin PhysicsJointLimits/PhysicsBodies attribute values\n"
  # Recurse through the tree
  childObjects = object.childObjects()
  for child in childObjects:
    print(child.Name)
    if child.isType("PhysicsJointLimit"):
      changeEndorphinJointLimitAttributes(child)
      print("joint limit")
    elif child.isType("PhysicsBody"):
      changeEndorphinBodyAttributes(child)
    changeEndorphinAttributes(child)

#############################################################################################
## Functions for exporting the animation collision bound information 
##  to xml(RS animation bound) format
#############################################################################################
#--------------------------------------------------------------------------------------------
def writeValueMatrixAnimationBounds(outputFile, matrix, indent):
  x1 = "%.9g" % matrix.xAxis().x()
  x2 = "%.9g" % matrix.xAxis().y()
  x3 = "%.9g" % matrix.xAxis().z()
  y1 = "%.9g" % matrix.yAxis().x()
  y2 = "%.9g" % matrix.yAxis().y()
  y3 = "%.9g" % matrix.yAxis().z()
  z1 = "%.9g" % matrix.zAxis().x()
  z2 = "%.9g" % matrix.zAxis().y()
  z3 = "%.9g" % matrix.zAxis().z()
  t1 = "%.9g" % matrix.translation().x()
  t2 = "%.9g" % matrix.translation().y()
  t3 = "%.9g" % matrix.translation().z()
  
  checkIsOrthonormal(matrix)

  outputFile.write(indent + '<m00> %s </m00>  <m01> %s </m01>  <m02> %s </m02>  <m03> %s </m03>\n' % (x1, y1, z1, "%.9g" % 0.0))
  outputFile.write(indent + '<m10> %s </m10>  <m11> %s </m11>  <m12> %s </m12>  <m13> %s </m13>\n' % (x2, y2, z2, "%.9g" % 0.0))
  outputFile.write(indent + '<m20> %s </m20>  <m21> %s </m21>  <m22> %s </m22>  <m23> %s </m23>\n' % (x3, y3, z3, "%.9g" % 0.0))
  outputFile.write(indent + '<m30> %s </m30>  <m31> %s </m31>  <m32> %s </m32>  <m33> %s </m33>\n' % (t1, t2, t3, "%.9g" % 1.0))

#--------------------------------------------------------------------------------------------
def writeAnimationBoundInfoTry(outputFile, physicsBody, indent):
  childObjects = physicsBody.childObjects()
  if len(childObjects) > 0:
    for childObject in childObjects:
      if childObject.isType('PhysicsBody'):
        writeAnimationBoundInfo(outputFile, nmx.PhysicsBody(nmx.PhysicsBody(childObject)), indent)
      else:
        print "ERROR: Root not a body, or Transform/PhysicsBodyTM doesn't have a child that is a PhysicsBody"

#--------------------------------------------------------------------------------------------
def writeAnimationBoundInfo(outputFile, physicsBody, indent):
  print("Writing Animation Bound Data %s" % endorphinToRockstarName(physicsBody.Name))
  volumeID = volumeIDPrefix + physicsBody.objectID()
  shapeID = shapeIDPrefix + physicsBody.objectID()
  transform = nmx.Transform(physicsBody.parent())                            
  physicsBody_WorldFrame = transform.WorldMatrix()                        
  physicsBody_LocalFrame = transform.LocalMatrix()                         

    
  outputFile.write('  <Part>\n')  
  outputFile.write('    <name>\n')  
  outputFile.write('      "%s"\n' % endorphinToRockstarName(physicsBody.Name))  
  #For Animals uncomment and comment line above  outputFile.write('      "%s"\n' % physicsBody.Name) 
  outputFile.write('    </name>\n')  
  outputFile.write('    <full_path>\n')  
  outputFile.write('      "%s"\n' % physicsBody.path())  
  outputFile.write('    </full_path>\n')  
  outputFile.write('    <world>\n')  
  if physicsBody.Shape.string() == "Capsule":
    writeValueMatrixAnimationBounds(outputFile, convertCapsuleMatrixToRS(physicsBody_WorldFrame), "      ")
  else: # "Box"or Sphere
    writeValueMatrixAnimationBounds(outputFile, physicsBody_WorldFrame.transpose3x3(), "      ")
  outputFile.write('    </world>\n')  
  outputFile.write('    <local>\n')  
  if physicsBody.Shape.string() == "Capsule":
    writeValueMatrixAnimationBounds(outputFile, convertCapsuleMatrixToRS(physicsBody_LocalFrame), "      ")
  else: # "Box"or Sphere
    writeValueMatrixAnimationBounds(outputFile, physicsBody_LocalFrame.transpose3x3(), "      ")
  outputFile.write('    </local>\n')  
  outputFile.write('    <Volume>\n')  
  if physicsBody.Shape.string() == "Capsule":
    outputFile.write('      <Capsule>\n')  
  elif physicsBody.Shape.string() == "Box":
    outputFile.write('      <Box>\n')
  else:
    print("ERROR:   not a capsule or a box")
  outputFile.write('        <relativeTransform>\n')  
  outputFile.write('          <m00> 1 </m00>  <m01> 0 </m01>  <m02> 0 </m02>  <m03> 0 </m03>\n')  
  outputFile.write('          <m10> 0 </m10>  <m11> 1 </m11>  <m12> 0 </m12>  <m13> 0 </m13>\n')  
  outputFile.write('          <m20> 0 </m20>  <m21> 0 </m21>  <m22> 1 </m22>  <m23> 0 </m23>\n')  
  outputFile.write('          <m30> 0 </m30>  <m31> 0 </m31>  <m32> 0 </m32>  <m33> 1 </m33>\n')  
  outputFile.write('        </relativeTransform>\n')  
  outputFile.write('        <name>\n')  
  outputFile.write('          "%s"\n' % endorphinToRockstarName(physicsBody.Name)) 
  #For Animals uncomment and comment line above  outputFile.write('          "%s"\n' % physicsBody.Name)   
  outputFile.write('        </name>\n')  
  outputFile.write('        <full_path>\n')  
  outputFile.write('          "%s"\n' % physicsBody.path())  
  outputFile.write('        </full_path>\n')  
  if physicsBody.Shape.string() == "Sphere":
    outputFile.write('        <radius>\n')  
    outputFile.write('          %s\n' % physicsBody.Radius)  
    outputFile.write('        </radius>\n')  
  elif physicsBody.Shape.string() == "Capsule":
    outputFile.write('        <radius>\n')  
    outputFile.write('          %s\n' % physicsBody.Radius)  
    outputFile.write('        </radius>\n')  
    outputFile.write('        <height>\n')  
    outputFile.write('          %s\n' % physicsBody.Length)  
    outputFile.write('        </height>\n')  
  elif physicsBody.Shape.string() == "Box":
    outputFile.write('        <dimensionX>\n')  
    outputFile.write('          %s\n' % physicsBody.Width)  
    outputFile.write('        </dimensionX>\n')  
    outputFile.write('        <dimensionY>\n')  
    outputFile.write('          %s\n' % physicsBody.Height)  
    outputFile.write('        </dimensionY>\n')  
    outputFile.write('        <dimensionZ>\n')  
    outputFile.write('          %s\n' % physicsBody.Length)  
    outputFile.write('        </dimensionZ>\n')  
  if physicsBody.Shape.string() == "Capsule":
    outputFile.write('      </Capsule>\n')  
  elif physicsBody.Shape.string() == "Box":
    outputFile.write('      </Box>\n')
  outputFile.write('    </Volume>\n')  
  outputFile.write('  </Part>\n')  
            
#--------------------------------------------------------------------------------------------
def recursivewriteAnimationBoundInfo(outputFile, object, indent):
    
  print "_" + object.Name.string()
  childObjects = object.childObjects()
  if len(childObjects) > 0:
    for childObject in childObjects:
      print indent + " child " + childObject.Name.string()
      jointObject = childObject;
      jointLimitName = childObject.Name.string() + "PhysicsJointLimit"
      for childObject in childObjects:
        if childObject.isType('PhysicsJointLimit') and jointLimitName == childObject.Name.string():
          jointChildObjects = jointObject.childObjects()
          for jointChildObject in jointChildObjects:
            bodyObject = jointChildObject;
            if jointChildObject.isType('Transform'):
              print jointChildObject.Name.string()
              writeAnimationBoundInfoTry(outputFile, jointChildObject, indent)
              recursivewriteAnimationBoundInfo(outputFile, jointObject, indent)

#--------------------------------------------------------------------------------------------
def writeAnimationCollisionBounds(outputFile, physicsCharacter):
  print("Writing Animation Bound Data")
  outputFile.write('<?xml version="1.0"?>\n')  
  outputFile.write('<MODEL>\n')  
  indent = ''
  physicsCharacterRoot =  physicsCharacter.childObjects()[0].childObjects()[0]
  print "physicsCharacterRoot: ", physicsCharacterRoot.Name.string()
  #write root info 1st 
  jointChildObjects = physicsCharacterRoot.childObjects()
  bodyTransformName = physicsCharacterRoot.Name.string() +"PhysicsBodyTM"
  if len(jointChildObjects) > 0:
    for jointChildObject in jointChildObjects:
      print "jointChildObject: ", jointChildObject.Name.string()
      if jointChildObject.isType('Transform') and bodyTransformName == jointChildObject.Name.string():
        print jointChildObject.Name.string()
        writeAnimationBoundInfoTry(outputFile, jointChildObject, indent)
  recursivewriteAnimationBoundInfo(outputFile, physicsCharacterRoot, indent)
  outputFile.write('</MODEL>\n')  

#--------------------------------------------------------------------------------------------
def exportAnimationCollisionBounds(physicsCharacter):
  "Export the Animation Collision Bounds in xml(R* animation bound) format"
  global double2Hex
  double2Hex = True
  fileName = physicsCharacter.OutputDirectory.string() + physicsCharacter.AssetName.string() + "_AnimationBounds.py"
  outputFile = open(fileName, "w")
  writeAnimationCollisionBounds(outputFile,physicsCharacter);
  outputFile.close()
  print "Exported animation bound information to ", fileName 


#############################################################################################
## Functions for orientating the endorphin JointLimits from 
##   RS joint limit max and mins(added RS joint attributes) 
#############################################################################################
#--------------------------------------------------------------------------------------------
def orientateJointLimitFromAttributes(physicsJointLimit):
  "Given the correct physicsJointLimit_WorldMovingFrame (this will have been orientated by the user to match an existing model)\n"\
  " 1) orientates an endorphin joint from RS parameters \n"\
  " 2) sets an endorphin joint's limits from R* parameters"

  #NB. Angles are set in radians not screen units
  print ":" + physicsJointLimit.Name.string()
  # Get the local frame, world frame and world moving frame of the physics joint limit.  
  physicsJoint = nmx.PhysicsJointLimit(physicsJointLimit).physicsJoint()                            
  physicsJointLimit_WorldFrame = physicsJointLimit.WorldMatrix()
  physicsJointLimit_WorldMovingFrame = nmx.Matrix(nmx.Quaternion(nmx.Vector3(physicsJointLimit.MovingOffsetX(), physicsJointLimit.MovingOffsetY(), physicsJointLimit.MovingOffsetZ())))
  physicsJointLimit_WorldMovingFrame.multiply(physicsJoint.WorldMatrix())
  #physicsJointLimit_LocalFrame = physicsJointLimit.LocalMatrix()
 
  if physicsJointLimit.LimitType.string() == "BallSocket":#threedof
    #Given the physicsJointLimit_WorldMovingFrame (this will have been orientated by the user to match an existing model)
    # and given the R* assymetric joint limits for the joint
    # orientate the physicsJointLimit_WorldFrame so that on export the R* assymetric joint limits are output
    
    #Swing range of endorphin joint from R* joint parameters
    swing1Angle = 0.5*(physicsJointLimit.Maxfirstleanangle.double() - physicsJointLimit.Minfirstleanangle.double())
    #print "swing1Angle=", swing1Angle
    swing2Angle = 0.5*(physicsJointLimit.Maxsecondleanangle.double() - physicsJointLimit.Minsecondleanangle.double())
    physicsJointLimit.Swing1Angle.setFloat(swing1Angle)
    physicsJointLimit.Swing2Angle.setFloat(swing2Angle)
    #Orientate the endorphin Physics Joint world matrix using R* joint parameters
    lean1Offset = 0.5*(physicsJointLimit.Maxfirstleanangle.double() + physicsJointLimit.Minfirstleanangle.double())
    #print "lean1Offset=", lean1Offset
    lean2Offset = 0.5*(physicsJointLimit.Maxsecondleanangle.double() + physicsJointLimit.Minsecondleanangle.double())
    limitWorldFrame = nmx.Matrix()
    limitWorldFrame.set(physicsJointLimit_WorldMovingFrame)
    limitWorldFrame.rotateByAxisAngle(limitWorldFrame.yAxis(), -lean1Offset)
    limitWorldFrame.rotateByAxisAngle(limitWorldFrame.zAxis(), -lean2Offset)
    nmx.PhysicsJointLimit(physicsJointLimit).setWorldMatrix(limitWorldFrame)
    #Twist range from R* joint parameters
    twistAngle = (physicsJointLimit.Maxtwistangle.double() - physicsJointLimit.Mintwistangle.double())
    twistOffset = -(0.5*(physicsJointLimit.Mintwistangle.double() + physicsJointLimit.Maxtwistangle.double()))
    physicsJointLimit.TwistAngle.setFloat(twistAngle)
    physicsJointLimit.TwistOffset.setFloat(twistOffset)
  elif physicsJointLimit.LimitType.string() == "Hinge":#onedof   
    #Twist range from R* joint parameters
    twistAngle = (physicsJointLimit.Maxangle.double() - physicsJointLimit.Minangle.double())
    twistOffset = (0.5*(physicsJointLimit.Maxangle.double() + physicsJointLimit.Minangle.double()))
    physicsJointLimit.TwistAngle.setFloat(twistAngle)
    physicsJointLimit.TwistOffset.setFloat(twistOffset)
    
#--------------------------------------------------------------------------------------------
def orientateAllJointLimitsFromAttributes(object):
  "Given the correct physicsJointLimit_WorldMovingFrame (this will have been orientated by the user to match an existing model)\n"\
  "Walks down the Character Rig tree and \n"\
  " 1) orientates endorphin joints from RS parameters \n"\
  " 2) sets endorphin joint limits from R* parameters"
  childObjects = object.childObjects()
  if len(childObjects) > 0:
    for childObject in childObjects:
      if childObject.isType('PhysicsJointLimit'):
        orientateJointLimitFromAttributes(childObject)
      orientateAllJointLimitsFromAttributes(childObject)  
      
#############################################################################################
## Functions for exporting the physics character in XML(121) format
#############################################################################################
#--------------------------------------------------------------------------------------------
def writeValueMatrix(outputFile, matrix, indent):
  "Write a matrix to outputFile in XML(121) format"
  x1 = matrix.xAxis().x()
  x2 = matrix.xAxis().y()
  x3 = matrix.xAxis().z()
  y1 = matrix.yAxis().x()
  y2 = matrix.yAxis().y()
  y3 = matrix.yAxis().z()
  z1 = matrix.zAxis().x()
  z2 = matrix.zAxis().y()
  z3 = matrix.zAxis().z()
  t1 = matrix.translation().x()
  t2 = matrix.translation().y()
  t3 = matrix.translation().z()
  
  checkIsOrthonormal(matrix)
  
  outputFile.write(indent + '<r0 c0="%s" c1="%s" c2="%s" c3="%s"/>\n' % (writeFloat(x1), writeFloat(x2), writeFloat(x3), writeFloat(0.0)))
  outputFile.write(indent + '<r1 c0="%s" c1="%s" c2="%s" c3="%s"/>\n' % (writeFloat(y1), writeFloat(y2), writeFloat(y3), writeFloat(0.0)))
  outputFile.write(indent + '<r2 c0="%s" c1="%s" c2="%s" c3="%s"/>\n' % (writeFloat(z1), writeFloat(z2), writeFloat(z3), writeFloat(0.0)))
  outputFile.write(indent + '<r3 c0="%s" c1="%s" c2="%s" c3="%s"/>\n' % (writeFloat(t1), writeFloat(t2), writeFloat(t3), writeFloat(1.0)))


#--------------------------------------------------------------------------------------------
def checkIsOrthonormal(matrix):
  "check if the matrix is orthonormal i.e. a valid ES transformation matrix"
  #To be a valid transformation matrix, the upper-left 3x3 sub-matrix must
  #be orthonormal, with a determinant of +1. 
  #(and the right-most column must be [0 0 0 1]) - this isn't checked for as we output this column hard coded to be [0 0 0 1] 
  tolerance = 0.0001 #taken from Euphoria studio
  
  #I'm copying the matrix just so I don't accidentaly change it using the matrix functions
  matrixCopy = nmx.Matrix()
  matrixT = nmx.Matrix()
  matrixT.set(matrix)
  matrixCopy.set(matrix)
  nmx.Matrix.transpose(matrixT)
  #even if 3x3 is identity, IsIdentity funcction returns false below if rest of matrix
  #is not initialized to be identity
  shouldBeI = nmx.Matrix()
  shouldBeI3x3 = nmx.Matrix()
  shouldBeI3x3 = matrixCopy.multiply(matrixT)
  shouldBeI.setXAxis(shouldBeI3x3.xAxis())
  shouldBeI.setYAxis(shouldBeI3x3.yAxis())
  shouldBeI.setZAxis(shouldBeI3x3.zAxis())
  if not(nmx.Matrix.isIdentity(shouldBeI,tolerance)): #matrix*matrixT should be identity if matrix is orthonormal
    print "**********ERROR matrix not orthornormal" 
    print nmx.Matrix.determinant(matrix)
  if (math.fabs(nmx.Matrix.determinant(matrix)) - 1.0 > tolerance):
    print "**********ERROR determinant not = +1...matrix not orthornormal" 

#--------------------------------------------------------------------------------------------
def writeValueFloat(outputFile, label, valueFloat, indent):
  "Write a float to outputFile in XML(121) format (hex value)"
  outputFile.write(indent + '<%s>%s</%s>\n' % (label, writeFloat(valueFloat), label))

#--------------------------------------------------------------------------------------------
def writeValueFloatFromObject(outputFile, label, valueFloat, indent):
  "Write a float to outputFile in XML(121) format (hex value)"
  valueFloat1 = float(valueFloat.string())
  outputFile.write(indent + '<%s>%s</%s>\n' % (label, writeFloat(valueFloat1), label))

#--------------------------------------------------------------------------------------------
def writeValueBool(outputFile, label, valueBool, indent):
  "Write a boolean to outputFile in XML(121) format"
  if valueBool == True:
    outputFile.write(indent + '<%s>true</%s>\n' % (label, label))
  else:
    outputFile.write(indent + '<%s>false</%s>\n' % (label, label))

#--------------------------------------------------------------------------------------------
def writeValueString(outputFile, label, valueString, indent):
  "Write a string to outputFile in XML(121) format"
  outputFile.write(indent + '<%s>%s</%s>\n' % (label, valueString, label))

#--------------------------------------------------------------------------------------------
def writeValueVector(outputFile, label, x, y, z, indent):
  "Write a vector to outputFile in XML(121) format"
  outputFile.write(indent + '<%s x="%s" y="%s" z="%s"/>\n' % (label, writeFloat(x), writeFloat(y), writeFloat(z)))

#--------------------------------------------------------------------------------------------
def writePhysicsJointLimitData(outputFile, physicsJointLimit, indent):
  "Write R* joint information to outputFile in XML(121) format - uses endorphin physicsJointLimit data"

  # Get the local frame, world frame and world moving frame of the physics joint limit.  
  physicsJoint = nmx.PhysicsJointLimit(physicsJointLimit).physicsJoint()                            
  physicsJointLimit_WorldFrame = physicsJointLimit.WorldMatrix()
  physicsJointLimit_WorldMovingFrame = nmx.Matrix(nmx.Quaternion(nmx.Vector3(physicsJointLimit.MovingOffsetX(), physicsJointLimit.MovingOffsetY(), physicsJointLimit.MovingOffsetZ())))
  physicsJointLimit_WorldMovingFrame.multiply(physicsJoint.WorldMatrix())
  #physicsJointLimit_LocalFrame = physicsJointLimit.LocalMatrix()
 
  euphoriaJointName = endorphinToRockstarName(physicsJointLimit.Name)
  jointID = jointIDPrefix + physicsJointLimit.objectID()
  if physicsJointLimit.LimitType.string() == "BallSocket":
    jointType = "threedof"
  elif physicsJointLimit.LimitType.string() == "Hinge":
    jointType = "onedof"
  print jointType   
  outputFile.write(indent + '<%s name="%s" id="00000000-0000-0000-0000-%s">\n' % (jointType, euphoriaJointName, jointID))
  if jointType == "unknown": 
    print "Unkown joint type"
  indent = indent + '  '  
  if jointType == "threedof":    
    #axis direction and leandirection define the plane that lean1 moves the body in.
    writeValueVector(outputFile, 'axisposition', physicsJointLimit_WorldFrame.translation().x(), physicsJointLimit_WorldFrame.translation().y(), physicsJointLimit_WorldFrame.translation().z(), indent)
    writeValueVector(outputFile, 'axisdirection', physicsJointLimit_WorldMovingFrame.xAxis().x(), physicsJointLimit_WorldMovingFrame.xAxis().y(), physicsJointLimit_WorldMovingFrame.xAxis().z(), indent)
    writeValueBool(outputFile, 'limitenabled', physicsJointLimit.Limitenabled.boolean(), indent)
    writeValueBool(outputFile, 'createeffector', physicsJointLimit.Createeffector.boolean(), indent)
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
    writeValueFloat(outputFile, 'minfirstleanangle', -halfSwing1 + lean1Offset, indent)
    writeValueFloat(outputFile, 'maxfirstleanangle', halfSwing1 + lean1Offset, indent)
    writeValueFloat(outputFile, 'minsecondleanangle', -halfSwing2 + lean2Offset, indent)
    writeValueFloat(outputFile, 'maxsecondleanangle', halfSwing2 + lean2Offset, indent)
    min = -0.5*physicsJointLimit.TwistAngle.double() - physicsJointLimit.TwistOffset.double()
    max =  0.5*physicsJointLimit.TwistAngle.double() - physicsJointLimit.TwistOffset.double()
    writeValueFloat(outputFile, 'mintwistangle', min, indent)
    writeValueFloat(outputFile, 'maxtwistangle', max, indent)
    writeValueBool(outputFile, 'reversefirstleanmotor', physicsJointLimit.Reversefirstleanmotor.boolean(), indent)
    writeValueBool(outputFile, 'reversesecondleanmotor', physicsJointLimit.Reversesecondleanmotor.boolean(), indent)
    writeValueBool(outputFile, 'reversetwistmotor', physicsJointLimit.Reversetwistmotor.boolean(), indent)
    writeValueFloat(outputFile, 'softlimitfirstleanmultiplier', physicsJointLimit.Softlimitfirstleanmultiplier.double(), indent)
    writeValueFloat(outputFile, 'softlimitsecondleanmultiplier', physicsJointLimit.Softlimitsecondleanmultiplier.double(), indent)
    writeValueFloat(outputFile, 'softlimittwistmultiplier', physicsJointLimit.Softlimittwistmultiplier.double(), indent)
    writeValueFloat(outputFile, 'defaultleanforcecap', physicsJointLimit.Defaultleanforcecap.double(), indent)
    writeValueFloat(outputFile, 'defaulttwistforcecap', physicsJointLimit.Defaulttwistforcecap.double(), indent)
    writeValueFloat(outputFile, 'defaultmusclestiffness', physicsJointLimit.Defaultmusclestiffness.double(), indent)
    writeValueFloat(outputFile, 'defaultmusclestrength', physicsJointLimit.Defaultmusclestrength.double(), indent)
    writeValueFloat(outputFile, 'defaultmuscledamping', physicsJointLimit.Defaultmuscledamping.double(), indent)
  elif jointType == "onedof":    
    writeValueVector(outputFile, 'axisposition', physicsJointLimit_WorldFrame.translation().x(), physicsJointLimit_WorldFrame.translation().y(), physicsJointLimit_WorldFrame.translation().z(), indent)
    writeValueVector(outputFile, 'axisdirection', physicsJointLimit_WorldFrame.xAxis().x(), physicsJointLimit_WorldFrame.xAxis().y(), physicsJointLimit_WorldFrame.xAxis().z(), indent)
    writeValueBool(outputFile, 'limitenabled', physicsJointLimit.Limitenabled.boolean(), indent)        
    #NB. Rotating the joint limit around the hinge axis has no effect (in endorphin either)
    #moving frame can be outside of limit offset - mmmmtodo WARN here as why would you want to do that?
    max = 0.5*physicsJointLimit.TwistAngle.double() + physicsJointLimit.TwistOffset.double()
    min = physicsJointLimit.TwistOffset.double() - 0.5*physicsJointLimit.TwistAngle.double()
    writeValueFloat(outputFile, 'minangle', min, indent)
    writeValueFloat(outputFile, 'maxangle', max, indent)
    writeValueBool(outputFile, 'createeffector', physicsJointLimit.Createeffector.boolean(), indent)
    writeValueFloat(outputFile, 'defaultleanforcecap', physicsJointLimit.Defaultleanforcecap.double(), indent)
    writeValueFloat(outputFile, 'defaultmusclestiffness', physicsJointLimit.Defaultmusclestiffness.double(), indent)
    writeValueFloat(outputFile, 'defaultmusclestrength', physicsJointLimit.Defaultmusclestrength.double(), indent)
    writeValueFloat(outputFile, 'defaultmuscledamping', physicsJointLimit.Defaultmuscledamping.double(), indent)
  
#--------------------------------------------------------------------------------------------
def writePhysicsBodyData(outputFile, physicsBody, indent):
  "Write R* articulatedPart information to outputFile in XML(121) format - uses endorphin physicsBody data"
  # Get the local frame and world frame of the transform that positions the physics body.
  transform = nmx.Transform(physicsBody.parent())                            
  physicsBody_WorldFrame = transform.WorldMatrix()                        
  #physicsBody_LocalFrame = transform.LocalMatrix()                         

  articulatedPartID = articulatedPartIDPrefix + physicsBody.objectID()
  volumeID = volumeIDPrefix + physicsBody.objectID()
  
  #articulated Body Part info
  outputFile.write(indent + '<articulatedpart name="%s" ' % endorphinToRockstarName(physicsBody.Name))
  outputFile.write('id="00000000-0000-0000-0000-%s">\n' % articulatedPartID)
  indent = indent + '  '
  writeValueBool(outputFile, 'hascollision', physicsBody.Hascollision.boolean(), indent)
  writeValueVector(outputFile, 'centreofmass', physicsBody.CentreofmassX.double(), physicsBody.CentreofmassY.double(), physicsBody.CentreofmassZ.double(), indent)
  writeValueFloatFromObject(outputFile, 'mass', physicsBody.Rs_mass, indent)
  writeValueFloat(outputFile, 'lineardamping', physicsBody.Lineardamping.double(), indent)
  writeValueFloat(outputFile, 'angulardamping', physicsBody.Angulardamping.double(), indent)
  writeValueFloatFromObject(outputFile, 'friction',  physicsBody.StaticFriction, indent)
  writeValueFloatFromObject(outputFile, 'restitution', physicsBody.Restitution, indent)
  writeValueString(outputFile, 'component', endorphinToRockstarName(physicsBody.Name), indent)
  outputFile.write(indent + '<transform>\n')
  print("WRITE PHYSICS BODY DATA")
  print(physicsBody.Name())
  if physicsBody.Shape.string() == "Capsule":
    writeValueMatrix(outputFile, convertCapsuleMatrixToRS(physicsBody_WorldFrame), indent + "  ")
  else: # "Box"or Sphere
    writeValueMatrix(outputFile, physicsBody_WorldFrame.transpose3x3(), indent + "  ")
  outputFile.write(indent + '</transform>\n')
  outputFile.write(indent + '<volume>00000000-0000-0000-0000-%s</volume>\n' % volumeID)
  writeValueFloat(outputFile, 'gravityscale', physicsBody.Gravityscale.double(), indent)
  writeValueFloat(outputFile, 'inertiamultiplier', physicsBody.Inertiamultiplier.double(), indent)
        
#--------------------------------------------------------------------------------------------
def writeVolumeData(outputFile, volume, indent):
  "Write R* volume information to outputFile in XML(121) format - uses endorphin physicsBody data"
  volumeID = volumeIDPrefix + volume.objectID()
  shapeID = shapeIDPrefix + volume.objectID()
  outputFile.write(indent + '<volume name="%s" id="00000000-0000-0000-0000-%s">\n' % (endorphinToRockstarName(volume.Name), volumeID))
  outputFile.write(indent + '  <%s name="%s" id="00000000-0000-0000-0000-%s">\n' % (volume.Shape.string().lower(), endorphinToRockstarName(volume.Name), shapeID))
  outputFile.write(indent + '    <relativetransform>\n')
  zero = writeFloat(0);
  one = writeFloat(1);
  outputFile.write(indent + '      <r0 c0="%s" c1="%s" c2="%s" c3="%s"/>\n' % (one, zero, zero, zero))
  outputFile.write(indent + '      <r1 c0="%s" c1="%s" c2="%s" c3="%s"/>\n' % (zero, one, zero, zero))
  outputFile.write(indent + '      <r2 c0="%s" c1="%s" c2="%s" c3="%s"/>\n' % (zero, zero, one, zero))
  outputFile.write(indent + '      <r3 c0="%s" c1="%s" c2="%s" c3="%s"/>\n' % (zero, zero, zero, one))
  outputFile.write(indent + '    </relativetransform>\n')
  # dimensions depending on shape type
  newIndent = indent + '    '
  if volume.Shape.string() == "Sphere":
    writeValueFloatFromObject(outputFile, 'radius', volume.Radius, newIndent)
  elif volume.Shape.string() == "Capsule":
    writeValueFloatFromObject(outputFile, 'radius', volume.Radius, newIndent)
    writeValueFloatFromObject(outputFile, 'height', volume.Length, newIndent)
  elif volume.Shape.string() == "Box":
    writeValueFloatFromObject(outputFile, 'dimensionx', volume.Width, newIndent)
    writeValueFloatFromObject(outputFile, 'dimensiony', volume.Height, newIndent)
    writeValueFloatFromObject(outputFile, 'dimensionz', volume.Length, newIndent)            
  outputFile.write(indent + '  </%s>\n' % volume.Shape.string().lower())
  outputFile.write(indent + '</volume>\n')

#--------------------------------------------------------------------------------------------
def writeAnimationRigBodyTransforms(outputFile, physicsBody, indent):
  "Write R* kinematicsRig/Maya transforms/Animation rig information to outputFile in XML(121) format\n"\
  " - uses endorphin physicsBody data\n"\
  " The local transformation data ONLY is used when making the *.xrt/*.prt/*.art files"
  
  volumeID = volumeIDPrefix + physicsBody.objectID()
  shapeID = shapeIDPrefix + physicsBody.objectID()
  transform = nmx.Transform(physicsBody.parent())                            
  physicsBody_WorldFrame = transform.WorldMatrix()                        
  physicsBody_LocalFrame = transform.LocalMatrix()                         

  outputFile.write(indent + '<mayatransform name="transform%s" id="00000000-0000-0000-0001-%s">\n' % (physicsBody.objectID(), volumeID))
  outputFile.write(indent + '  <jointorient x="000000003ff00000" y="0000000000000000" z="0000000000000000" w="0000000000000000"/>\n')
  outputFile.write(indent + '  <rotateorient x="000000003ff00000" y="0000000000000000" z="0000000000000000" w="0000000000000000"/>\n')
  outputFile.write(indent + '  <scale x="000000003ff00000" y="000000003ff00000" z="000000003ff00000"/>\n')
  outputFile.write(indent + '  <translate x="40000000bf8bdf8f" y="80000000bf6dfb93" z="e00000003ee0d9f4"/>\n')
  outputFile.write(indent + '  <rotate x="200000003fe6a0a5" y="800000003fe6a096" z="0000000000000000" w="0000000000000000"/>\n')
  outputFile.write(indent + '  <world>\n')
  print("WRITE ANIMATION RIG BODY DATA")
  print(physicsBody.Name())
  if physicsBody.Shape.string() == "Capsule":
    writeValueMatrix(outputFile, convertCapsuleMatrixToRS(physicsBody_WorldFrame), indent + "    ")
  else: # "Box"or Sphere
    writeValueMatrix(outputFile, physicsBody_WorldFrame.transpose3x3(), indent + "    ")
  outputFile.write(indent + '  </world>\n')
  outputFile.write(indent + '  <local>\n')
  if physicsBody.Shape.string() == "Capsule":
    writeValueMatrix(outputFile, convertCapsuleMatrixToRS(physicsBody_LocalFrame), indent + "    ")
  else: # "Box"or Sphere
    writeValueMatrix(outputFile, physicsBody_LocalFrame.transpose3x3(), indent + "    ")
  outputFile.write(indent + '  </local>\n')
  outputFile.write(indent + '  <childshapes>\n')
  outputFile.write(indent + '    <childshape>00000000-0000-0000-0000-%s</childshape>\n' % shapeID)
  outputFile.write(indent + '  </childshapes>\n')
  outputFile.write(indent + '  <childjoints/>\n')
  outputFile.write(indent + '  <childvolumes/>\n')
  outputFile.write(indent + '  <dofx>true</dofx>\n')
  outputFile.write(indent + '  <dofy>true</dofy>\n')
  outputFile.write(indent + '  <dofz>true</dofz>\n')
  outputFile.write(indent + '</mayatransform>\n')
            
#--------------------------------------------------------------------------------------------
def writeAnimationRigJointTransforms(outputFile, physicsJointLimit, indent):
  "Write R* kinematicsRig/Maya transforms/Animation rig information to outputFile in XML(121) format\n"\
  " None of this is used when making the *.xrt/*.prt/*.art files"
  jointID = jointIDPrefix + physicsJointLimit.objectID()
  outputFile.write(indent + '<mayatransform name="transform%s" id="00000000-0000-0000-0002-%s">\n' % (physicsJointLimit.objectID(), jointID))
  outputFile.write(indent + '  <jointorient x="000000003ff00000" y="0000000000000000" z="0000000000000000" w="0000000000000000"/>\n')
  outputFile.write(indent + '  <rotateorient x="000000003ff00000" y="0000000000000000" z="0000000000000000" w="0000000000000000"/>\n')
  outputFile.write(indent + '  <scale x="000000003ff00000" y="000000003ff00000" z="000000003ff00000"/>\n')
  outputFile.write(indent + '  <translate x="40000000bf8bdf8f" y="80000000bf6dfb93" z="e00000003ee0d9f4"/>\n')
  outputFile.write(indent + '  <rotate x="c00000003fefec6b" y="00000000bececc56" z="c0000000bed38552" w="e00000003fb1b0b1"/>\n')
  outputFile.write(indent + '  <world>\n')
  outputFile.write(indent + '    <r0 c0="200000003f932188" c1="200000003f9967b0" c2="200000003feffc0b" c3="0000000000000000"/>\n')
  outputFile.write(indent + '    <r1 c0="c00000003f732553" c1="c00000003feffd5c" c2="e0000000bf997fa5" c3="0000000000000000"/>\n')
  outputFile.write(indent + '    <r2 c0="00000000bfeffe7c" c1="c00000003f750ace" c2="c00000003f93018c" c3="0000000000000000"/>\n')
  outputFile.write(indent + '    <r3 c0="40000000bfbee829" c1="400000003f5fd6d5" c2="e0000000bfda9b38" c3="000000003ff00000"/>\n')
  outputFile.write(indent + '  </world>\n')
  outputFile.write(indent + '  <local>\n')
  outputFile.write(indent + '    <r0 c0="400000003fefb1c6" c1="00000000bfc1a5db" c2="40000000bee489f5" c3="0000000000000000"/>\n')
  outputFile.write(indent + '    <r1 c0="000000003fc1a5db" c1="200000003fefb1c2" c2="e00000003edc095c" c3="0000000000000000"/>\n')
  outputFile.write(indent + '    <r2 c0="e00000003ee268f6" c1="60000000bee0b612" c2="e00000003feffffd" c3="0000000000000000"/>\n')
  outputFile.write(indent + '    <r3 c0="e00000003c900000" c1="600000003c300000" c2="0000000000000000" c3="000000003ff00000"/>\n')
  outputFile.write(indent + '  </local>\n')
  outputFile.write(indent + '  <childshapes/>\n')
  outputFile.write(indent + '  <childjoints>\n')
  outputFile.write(indent + '    <childjoint>00000000-0000-0000-0000-%s</childjoint>\n' % jointID)
  outputFile.write(indent + '  </childjoints>\n')
  outputFile.write(indent + '  <childvolumes/>\n')
  outputFile.write(indent + '  <dofx>true</dofx>\n')
  outputFile.write(indent + '  <dofy>true</dofy>\n')
  outputFile.write(indent + '  <dofz>true</dofz>\n')
  outputFile.write(indent + '</mayatransform>\n')
              
#--------------------------------------------------------------------------------------------
def recursiveWriteVolumeData(outputFile, object, indent):
    
  childObjects = object.childObjects()
  if len(childObjects) > 0:
    for childObject in childObjects:
      if childObject.isType('PhysicsBody'):
        writeVolumeData(outputFile, nmx.PhysicsBody(nmx.PhysicsBody(childObject)), indent)
      recursiveWriteVolumeData(outputFile, childObject, indent)

#--------------------------------------------------------------------------------------------
def recursivewriteAnimationRigBodyTransforms(outputFile, object, indent):
    
  childObjects = object.childObjects()
  if len(childObjects) > 0:
    for childObject in childObjects:
      if childObject.isType('PhysicsBody'):
        writeAnimationRigBodyTransforms(outputFile, nmx.PhysicsBody(nmx.PhysicsBody(childObject)), indent)
      recursivewriteAnimationRigBodyTransforms(outputFile, childObject, indent)

#--------------------------------------------------------------------------------------------
def recursivewriteAnimationRigJointTransforms(outputFile, object, indent):
    
  childObjects = object.childObjects()
  if len(childObjects) > 0:
    for childObject in childObjects:
      if childObject.isType('PhysicsJointLimit'):
        writeAnimationRigJointTransforms(outputFile, nmx.PhysicsBody(nmx.PhysicsBody(childObject)), indent)
      recursivewriteAnimationRigJointTransforms(outputFile, childObject, indent)

#--------------------------------------------------------------------------------------------
def writeRootData(outputFile, object, indent):
  childObjects = object.childObjects()[0].childObjects()[0].childObjects()
  if len(childObjects) > 0:
    object = childObjects[0]
    for childObject in childObjects:
      if childObject.isType('Transform'):
        print childObject.Name
        writeArticulatedPartData(outputFile, childObject, indent)
        

#--------------------------------------------------------------------------------------------
def writeArticulatedPartData(outputFile, object, indent):
    
  childObjects = object.childObjects()
  if len(childObjects) > 0:
    newIndent = indent
    for childObject in childObjects:
      if childObject.isType('PhysicsBody'):
        writePhysicsBodyData(outputFile, nmx.PhysicsBody(nmx.PhysicsBody(childObject)), newIndent)
      else:
        print "ERROR: Root not a body, or Transform/PhysicsBodyTM doesn't have a child that is a PhysicsBody"

#--------------------------------------------------------------------------------------------
def recursiveWritePhysicsCharacterData(outputFile, object, indent):
    
  print "_" + object.Name.string()
  childObjects = object.childObjects()
  if len(childObjects) > 0:
    indent = indent + '  '
    for childObject in childObjects:
      print indent + " child " + childObject.Name.string()
      jointObject = childObject;
      jointLimitName = childObject.Name.string() + "PhysicsJointLimit"      
      for childObject in childObjects:
        if childObject.isType('PhysicsJointLimit') and jointLimitName == childObject.Name.string():
          print indent + " CHILD " + childObject.Name.string()
          print indent + "   joint " + childObject.Name.string()
          writePhysicsJointLimitData(outputFile, nmx.PhysicsJointLimit(nmx.PhysicsJointLimit(childObject)), indent)
          jointType = "unknown"
          if nmx.PhysicsJointLimit(nmx.PhysicsJointLimit(childObject)).LimitType.string() == "BallSocket":
            jointType = "threedof"
          elif nmx.PhysicsJointLimit(nmx.PhysicsJointLimit(childObject)).LimitType.string() == "Hinge":
            jointType = "onedof"
          jointChildObjects = jointObject.childObjects()
          for jointChildObject in jointChildObjects:
            bodyObject = jointChildObject;
            print indent + "     jointChildObject " + jointChildObject.Name.string()
            if jointChildObject.isType('Transform'):
              indentNew = indent + '  '
              print indentNew + "   JOINTCHILDOBJECT " + jointChildObject.Name.string()
              writeArticulatedPartData(outputFile, jointChildObject, indentNew)
              recursiveWritePhysicsCharacterData(outputFile, jointObject, indentNew)
              outputFile.write(indentNew + '</articulatedpart>\n')
              outputFile.write(indent + '</%s>\n' % jointType)

#--------------------------------------------------------------------------------------------
def writeCollisionSets(outputFile, indent):

  allCollisionSets = nmx.ObjectArray()
  nmx.ObjectGroup(nmx.scene().CollisionGroups).childGroups(allCollisionSets)
  
  outputFile.write(indent + '<collisiongroup name="CollisionGroup" id="8fa0dd58-68ba-446e-940f-4ec195fe8954">\n')
      
  for collisionSet in allCollisionSets:
    collisionSetID = collisionSetIDPrefix + collisionSet.objectID()
    outputFile.write(indent + '  <collisionset name="%s" id="00000000-0000-0000-0000-%s">\n' % (collisionSet.Label, collisionSetID))
    outputFile.write(indent + '    <parts>\n')
    group = nmx.CollisionGroup(collisionSet)
    physicsJoints = nmx.ObjectArray()
    group.physicsJoints(physicsJoints)
    newIndent = indent + '      '
    for joint in physicsJoints:
      body = nmx.PhysicsJoint(joint).physicsBody()
      partID = articulatedPartIDPrefix + body.objectID()
      partIDstr = '00000000-0000-0000-0000-%s' % partID
      print partIDstr
      writeValueString(outputFile, 'part', partIDstr, newIndent)
    outputFile.write(indent + '    </parts>\n')
    outputFile.write(indent + '  </collisionset>\n')

  outputFile.write(indent + '</collisiongroup>\n')

#--------------------------------------------------------------------------------------------
def writePhysicsCharacterData(outputFile, physicsCharacter):
 
  outputFile.write('<?xml version="1.0" encoding="UTF-8"?>\n')  
  outputFile.write('<rockstardb master_version="1" representation_version="1" db_version="10" name="Character:RockstarDB:%s"> \n' % physicsCharacter.AssetName.string())  
  outputFile.write('  <rockstardbroot name="Root" id="00000000-0000-0000-0000-000000000001">\n')  
  outputFile.write('    <clump name="clump" id="00000000-0000-0000-0000-000000000002">\n')  
  outputFile.write('      <typeid>0</typeid>\n')  

  indent = '      '
  #output atriculated parts and joints
  writeRootData(outputFile, physicsCharacter, indent)  
  physicsCharacterPastRoot =  physicsCharacter.childObjects()[0].childObjects()[0]
  print "root name " + physicsCharacterPastRoot.Name()
  recursiveWritePhysicsCharacterData(outputFile, physicsCharacterPastRoot, indent)  
  outputFile.write('      </articulatedpart>\n')  
  #output collisions sets
  writeCollisionSets(outputFile, indent)
  #Maya transform
  volumeID = volumeIDPrefix
  shapeID = shapeIDPrefix
  #None of this is used to make the *.xrt/*.prt/*.art files
  outputFile.write(indent + '<mayatransform name="RootDummy" id="00000000-0000-0000-0002-%s">\n' % volumeID)
  outputFile.write(indent + '  <jointorient x="000000003ff00000" y="0000000000000000" z="0000000000000000" w="0000000000000000"/>\n')
  outputFile.write(indent + '  <rotateorient x="000000003ff00000" y="0000000000000000" z="0000000000000000" w="0000000000000000"/>\n')
  outputFile.write(indent + '  <scale x="000000003ff00000" y="000000003ff00000" z="000000003ff00000"/>\n')
  outputFile.write(indent + '  <translate x="40000000bf8bdf8f" y="80000000bf6dfb93" z="e00000003ee0d9f4"/>\n')
  outputFile.write(indent + '  <rotate x="200000003fe6a0a5" y="800000003fe6a096" z="0000000000000000" w="0000000000000000"/>\n')
  outputFile.write(indent + '  <world>\n')
  outputFile.write(indent + '    <r0 c0="000000003c8fffab" c1="00000000bff00000" c2="c00000003ebe0000" c3="0000000000000000"/>\n')
  outputFile.write(indent + '    <r1 c0="e00000003cb00001" c1="c0000000bebe0000" c2="00000000bff00000" c3="0000000000000000"/>\n')
  outputFile.write(indent + '    <r2 c0="000000003ff00000" c1="80000000bb9e660c" c2="c00000003ca7fffb" c3="0000000000000000"/>\n')
  outputFile.write(indent + '    <r3 c0="c0000000bee0c374" c1="80000000bf6dfb93" c2="40000000bf8bdf8f" c3="000000003ff00000"/>\n')
  outputFile.write(indent + '  </world>\n')
  outputFile.write(indent + '  <local>\n')
  outputFile.write(indent + '    <r0 c0="000000003ff00000" c1="e0000000385ffffe" c2="c0000000b95fffff" c3="0000000000000000"/>\n')
  outputFile.write(indent + '    <r1 c0="c0000000b9479da0" c1="800000003ee44406" c2="00000000bff00000" c3="0000000000000000"/>\n')
  outputFile.write(indent + '    <r2 c0="c0000000b9400000" c1="000000003ff00000" c2="800000003ee44406" c3="0000000000000000"/>\n')
  outputFile.write(indent + '    <r3 c0="40000000bf8bdf8f" c1="80000000bf6dfb93" c2="e00000003ee0d9f4" c3="000000003ff00000"/>\n')
  outputFile.write(indent + '  </local>\n')
  outputFile.write(indent + '  <childshapes/>\n')
  outputFile.write(indent + '  <childjoints/>\n')
  outputFile.write(indent + '  <childvolumes/>\n')
  outputFile.write(indent + '  <dofx>true</dofx>\n')
  outputFile.write(indent + '  <dofy>true</dofy>\n')
  outputFile.write(indent + '  <dofz>true</dofz>\n')
  indent = indent + '  '
  #The local transformation data ONLY is used when making the *.xrt/*.prt/*.art files"
  recursivewriteAnimationRigBodyTransforms(outputFile, physicsCharacter, indent)
  #None of this is used to make the *.xrt/*.prt/*.art files
  recursivewriteAnimationRigJointTransforms(outputFile, physicsCharacter, indent)
  outputFile.write('      </mayatransform>\n')
  
  #output volumes
  recursiveWriteVolumeData(outputFile, physicsCharacter, indent) 
 
  outputFile.write('    </clump>\n')  
  outputFile.write('  </rockstardbroot>\n')  
  outputFile.write('</rockstardb>\n')  

#--------------------------------------------------------------------------------------------
def recursivewriteNMBodyPart(outputFile, parentJoint, indent):
  global jointNumber    
  print indent + "JOINT %i "  % jointNumber + parentJoint.Name.string()
  parentJoint_childObjects = parentJoint.childObjects()
  if len(parentJoint_childObjects) > 0:
    for childObject in parentJoint_childObjects:
      if childObject.isType('PhysicsJoint'):
        for transform in childObject.childObjects():
         if transform.isType('Transform'):
           print indent + "PART %i "  % jointNumber + transform.Name.string()
           writeNMBodyPart(outputFile, transform, indent, jointNumber)       
           jointNumber = jointNumber + 1
        recursivewriteNMBodyPart(outputFile, childObject, indent)

#--------------------------------------------------------------------------------------------
def recursivewriteNMJoint(outputFile, parentJoint, indent, parentIndex):
  global jointNumber    
  print indent + parentJoint.Name.string()
  parentJoint_childObjects = parentJoint.childObjects()
  jointHadJointLimit = 0
  if len(parentJoint_childObjects) > 0:
    for childObject in parentJoint_childObjects:
      if childObject.isType('PhysicsJoint'):
        jointLimitName = childObject.Name.string() + "PhysicsJointLimit"
        for limit in parentJoint_childObjects:
         if limit.isType('PhysicsJointLimit') and jointLimitName == limit.Name.string():
           print indent + "JOINT %i "  % jointNumber + parentJoint.Name.string()
           print indent + "PARENTINDEX %i "  % parentIndex + parentJoint.Name.string()
           print indent + "JOINTLIMT %i "  % jointNumber + limit.Name.string()
           writeNMJoint(outputFile, limit, indent, jointNumber, parentIndex)
           jointNumber = jointNumber + 1
           jointHadJointLimit = 1
        recursivewriteNMJoint(outputFile, childObject, indent, jointHadJointLimit*jointNumber)
        jointHadJointLimit = 0

#--------------------------------------------------------------------------------------------
def writeNMBodyPart(outputFile, object, indentOrig, jointNumber):
  "writeNMBodyPart"
  global rootPos
  childObjects = object.childObjects()#if 2 error
  physicsBody = childObjects[0]
  if not physicsBody.isType('PhysicsBody'):
    print "ERROR: Root not a body, or Transform/PhysicsBodyTM doesn't have a child that is a PhysicsBody"

  # Get the local frame and world frame of the transform that positions the physics body.
  transform = nmx.Transform(physicsBody.parent())                            
  physicsBody_WorldFrame = transform.WorldMatrix() 
  #all relative to root
  offsetPos = nmx.Vector3()
  offsetPos.set(physicsBody_WorldFrame.translation())
  offsetPos.subtract(rootPos)
  physicsBody_WorldFrame.setTranslation(offsetPos);                      
  
  articulatedPartID = articulatedPartIDPrefix + physicsBody.objectID()
  volumeID = volumeIDPrefix + physicsBody.objectID()
  
  #articulated Body Part info
  outputFile.write(indentOrig + '<bodyPart name="%s">\n' % (endorphinToRockstarName(physicsBody.Name)))
  print(physicsBody.Name())
  indent = indentOrig + '  '
  writeValueString(outputFile, 'partNumber', '%i' % jointNumber, indent) 
  outputFile.write(indent + '<transform>\n')
  if physicsBody.Shape.string() == "Capsule":
    writeValueMatrix(outputFile, convertCapsuleMatrixToRS(physicsBody_WorldFrame), indent + "  ")
  else: # "Box"or Sphere
    writeValueMatrix(outputFile, physicsBody_WorldFrame.transpose3x3(), indent + "  ")
  outputFile.write(indent + '</transform>\n')

  outputFile.write(indentOrig + '</bodyPart>\n')
 

#--------------------------------------------------------------------------------------------
def writeNMJoint(outputFile, physicsJointLimit, indentOrig, jointNumber, parentIndex):
  "writeNMJoint"

  # Get the local frame, world frame and world moving frame of the physics joint limit.  
  physicsJoint = nmx.PhysicsJointLimit(physicsJointLimit).physicsJoint()                            
  physicsJointLimit_WorldFrame = physicsJointLimit.WorldMatrix()
  physicsJointLimit_WorldMovingFrame = nmx.Matrix(nmx.Quaternion(nmx.Vector3(physicsJointLimit.MovingOffsetX(), physicsJointLimit.MovingOffsetY(), physicsJointLimit.MovingOffsetZ())))
  physicsJointLimit_WorldMovingFrame.multiply(physicsJoint.WorldMatrix())
  #physicsJointLimit_LocalFrame = physicsJointLimit.LocalMatrix()

  euphoriaJointName = endorphinToRockstarName(physicsJointLimit.Name)
  if physicsJointLimit.LimitType.string() == "BallSocket":
    jointType = "threedof"
  elif physicsJointLimit.LimitType.string() == "Hinge":
    jointType = "onedof"
  else:
    jointType = "UNKNOWN"
  print jointType, euphoriaJointName   
  outputFile.write(indentOrig + '<%s name="%s">\n' % (jointType, euphoriaJointName))
  indent = indentOrig + '  ' 
  writeValueString(outputFile, 'jointNumber', '%i' % jointNumber, indent) 
  writeValueString(outputFile, 'parentIndex', '%i' % parentIndex, indent) 
  if jointType == "threedof":    
    #axis direction and leandirection define the plane that lean1 moves the body in.
    halfSwing1 = physicsJointLimit.Swing1Angle.double()
    halfSwing2 = physicsJointLimit.Swing2Angle.double()
    lean2Offset = math.copysign(1, physicsJointLimit_WorldMovingFrame.xAxis().dotProduct(physicsJointLimit_WorldFrame.yAxis())) * acosSafe(physicsJointLimit_WorldMovingFrame.yAxis().dotProduct(physicsJointLimit_WorldFrame.yAxis()))
    #print "halfSwing1 ", halfSwing1
    #print "halfSwing2 ", halfSwing2
    #print "lean2Offset ", lean2Offset
    rotatedWorlFrame = nmx.Matrix()
    rotatedWorlFrame.set(physicsJointLimit_WorldFrame)
    #Gave a slight error for lean1min/max if lean1 and lean2 limits asymmetric e.g shoulder, hip
    #rotatedWorlFrame.rotateByAxisAngle(physicsJointLimit_WorldMovingFrame.zAxis(),lean2Offset)
    rotatedWorlFrame.rotateByAxisAngle(rotatedWorlFrame.zAxis(),lean2Offset)
    lean1Offset = math.copysign(1, physicsJointLimit_WorldMovingFrame.zAxis().dotProduct(rotatedWorlFrame.xAxis())) * acosSafe(physicsJointLimit_WorldMovingFrame.xAxis().dotProduct(rotatedWorlFrame.xAxis()))
    #print 'minfirstleanangle'
    #print "-halfSwing1 + lean1Offset ", -halfSwing1 + lean1Offset
    #print "outputFile ", outputFile
    #print "indent ", indent
    writeValueFloat(outputFile, 'minfirstleanangle', -halfSwing1 + lean1Offset, indent)
    writeValueFloat(outputFile, 'maxfirstleanangle', halfSwing1 + lean1Offset, indent)
    writeValueFloat(outputFile, 'minsecondleanangle', -halfSwing2 + lean2Offset, indent)
    writeValueFloat(outputFile, 'maxsecondleanangle', halfSwing2 + lean2Offset, indent)
    min = -0.5*physicsJointLimit.TwistAngle.double() - physicsJointLimit.TwistOffset.double()
    max =  0.5*physicsJointLimit.TwistAngle.double() - physicsJointLimit.TwistOffset.double()
    writeValueFloat(outputFile, 'mintwistangle', min, indent)
    writeValueFloat(outputFile, 'maxtwistangle', max, indent)
    writeValueBool(outputFile, 'reversefirstleanmotor', physicsJointLimit.Reversefirstleanmotor.boolean(), indent)
    writeValueBool(outputFile, 'reversesecondleanmotor', physicsJointLimit.Reversesecondleanmotor.boolean(), indent)
    writeValueBool(outputFile, 'reversetwistmotor', physicsJointLimit.Reversetwistmotor.boolean(), indent)
    writeValueFloat(outputFile, 'softlimitfirstleanmultiplier', physicsJointLimit.Softlimitfirstleanmultiplier.double(), indent)
    writeValueFloat(outputFile, 'softlimitsecondleanmultiplier', physicsJointLimit.Softlimitsecondleanmultiplier.double(), indent)
    writeValueFloat(outputFile, 'softlimittwistmultiplier', physicsJointLimit.Softlimittwistmultiplier.double(), indent)
    writeValueFloat(outputFile, 'defaultleanforcecap', physicsJointLimit.Defaultleanforcecap.double(), indent)
    writeValueFloat(outputFile, 'defaulttwistforcecap', physicsJointLimit.Defaulttwistforcecap.double(), indent)
    writeValueFloat(outputFile, 'defaultmusclestiffness', physicsJointLimit.Defaultmusclestiffness.double(), indent)
    writeValueFloat(outputFile, 'defaultmusclestrength', physicsJointLimit.Defaultmusclestrength.double(), indent)
    writeValueFloat(outputFile, 'defaultmuscledamping', physicsJointLimit.Defaultmuscledamping.double(), indent)
  elif jointType == "onedof":    
    #NB. Rotating the joint limit around the hinge axis has no effect (in endorphin either)
    #moving frame can be outside of limit offset - mmmmtodo WARN here as why would you want to do that?
    max = 0.5*physicsJointLimit.TwistAngle.double() + physicsJointLimit.TwistOffset.double()
    min = physicsJointLimit.TwistOffset.double() - 0.5*physicsJointLimit.TwistAngle.double()
    writeValueFloat(outputFile, 'minangle', min, indent)
    writeValueFloat(outputFile, 'maxangle', max, indent)
    writeValueFloat(outputFile, 'defaultleanforcecap', physicsJointLimit.Defaultleanforcecap.double(), indent)
    writeValueFloat(outputFile, 'defaultmusclestiffness', physicsJointLimit.Defaultmusclestiffness.double(), indent)
    writeValueFloat(outputFile, 'defaultmusclestrength', physicsJointLimit.Defaultmusclestrength.double(), indent)
    writeValueFloat(outputFile, 'defaultmuscledamping', physicsJointLimit.Defaultmuscledamping.double(), indent)
 
  outputFile.write(indentOrig + '</%s>\n' % jointType)
 
#--------------------------------------------------------------------------------------------
def writePhysicsCharacterDataNMXML(outputFile, physicsCharacter):
  global jointNumber
  global rootPos
  #find root position
  rootBody = nmx.scene().object("|Scene|character|physicsCharacter|SKEL_ROOT|SKEL_Pelvis|SKEL_PelvisPhysicsBodyTM|SKEL_PelvisPhysicsBody")
  transform = nmx.Transform(rootBody.parent())                            
  physicsBody_WorldFrame = transform.WorldMatrix() 
  #all relative to root (therefore root body position = zero)
  rootPos.set(physicsBody_WorldFrame.translation())

  outputFile.write('<?xml version="1.0" encoding="UTF-8"?>\n')  
  outputFile.write('<rockstar master_version="1" representation_version="0">\n')  
  indent = '  '
  writeValueString(outputFile, 'name', physicsCharacter.AssetName.string(), indent) 
  writeValueString(outputFile, 'id', '%i' % physicsCharacter.AssetTypeId.integer(), indent)

  #output atriculated parts and joints
  print "Write Physics Data for NM ONLY xml from " + physicsCharacter.path()
  jointNumber = 0
  recursivewriteNMJoint(outputFile, physicsCharacter, indent, 0)
  jointNumber = 0
  recursivewriteNMBodyPart(outputFile, physicsCharacter, indent)
  outputFile.write('</rockstar>\n')  
  
#--------------------------------------------------------------------------------------------
def exportNMXML(physicsCharacter):
  "Export the NM only XML"
  global double2Hex
  double2Hex = False
  fileName = physicsCharacter.OutputDirectory.string() + physicsCharacter.AssetName.string() + "_NM.xml"
  outputFile = open(fileName, "w")
  writePhysicsCharacterDataNMXML(outputFile, physicsCharacter)
  outputFile.close()
  print "Exported character NM only info to ", fileName 

#--------------------------------------------------------------------------------------------
def exportPhysicsCharacter(physicsCharacter):
  "Export the physicsCharacter in xml(121) format"
  global double2Hex
  double2Hex = True
  fileName = physicsCharacter.OutputDirectory.string() + physicsCharacter.AssetName.string() + ".xml"
  outputFile = open(fileName, "w")
  writePhysicsCharacterData(outputFile, physicsCharacter)
  outputFile.close()
  print "Exported character physics model to ", fileName

#############################################################################################
## Functions for orientating the physics bodies and joints from 
##   from the added RS body attributes and added RS joint attributes 
#############################################################################################
#--------------------------------------------------------------------------------------------
def orientatePhysicsBodyFromDefaultBy2WorldAxis(physicsBody,Body_BoneAxis):
  "orientate endorphin physicsBodies from the added RS body attributes:\n"\
  "  BoneAxisIndex,AlignAxisIndex and WorldAlignAxisIndex"
  #Bodies are orientated 
  # 1)with a prescribed body axis (BoneAxisIndex) down in the direction of an input world vector (Body_BoneAxis)
  # 2)with a prescribed body axis (AlignAxisIndex) aligned to a prescribed world axis (AlignAxisIndex) 
  transform = nmx.Transform(physicsBody.parent())                            
  worldFrame = nmx.Matrix() #gives identity
  worldAxes = (worldFrame.xAxis(),worldFrame.yAxis(),worldFrame.zAxis())
  rockstarName = endorphinToRockstarName(physicsBody.Name)
  
  #Get alignment from endorphinDefault
  BoneAxisIndex = physicsBody.BoneAxisIndex.integer()
  BoneSign = math.copysign(1, BoneAxisIndex)
  BoneAxisIndex = abs(BoneAxisIndex) - 1#Convert x,y,z = 1,2,3 to 0,1,2
  
  AlignAxisIndex = physicsBody.AlignAxisIndex.integer()
  AlignSign = math.copysign(1, AlignAxisIndex)
  AlignAxisIndex = abs(AlignAxisIndex) - 1
  
  WorldAlignAxisIndex = physicsBody.WorldAlignAxisIndex.integer()
  AlignSign *= math.copysign(1, WorldAlignAxisIndex)
  WorldAlignAxisIndex = abs(WorldAlignAxisIndex) - 1
   
  OtherAxisIndex = (BoneAxisIndex + 1) % 3
  if OtherAxisIndex == AlignAxisIndex:
    OtherAxisIndex = (OtherAxisIndex + 1) % 3
    
  World_AlignedAxis = worldAxes[WorldAlignAxisIndex]

  Body_BoneAxis.normalize()
  Body_BoneAxis.scale(BoneSign)

  Body_AlignedAxis = nmx.Vector3()
  Body_AlignedAxis.crossProduct(Body_BoneAxis, World_AlignedAxis)
  Body_AlignedAxis.crossProduct(Body_BoneAxis)
  Body_AlignedAxis.normalize()
  Body_AlignedAxis.scale(AlignSign)
  
  Body_OtherAxis = nmx.Vector3()
  Body_OtherAxis.crossProduct(Body_BoneAxis, Body_AlignedAxis)
  Body_OtherAxis.normalize()
  if ((BoneAxisIndex + 1) % 3) != AlignAxisIndex:
    Body_OtherAxis.scale(-1.0)
  
  bodyAxes = {BoneAxisIndex : Body_BoneAxis}
  bodyAxes[AlignAxisIndex] = Body_AlignedAxis
  bodyAxes[OtherAxisIndex] = Body_OtherAxis
  worldFrame.setXAxis(bodyAxes[0])
  worldFrame.setYAxis(bodyAxes[1])
  worldFrame.setZAxis(bodyAxes[2])
  worldFrame.setTranslation(transform.worldTranslation())
  transform.setWorldMatrix(worldFrame)

#--------------------------------------------------------------------------------------------
def orientatePhysicsBodyFromDefault(physicsBody):
  "orientate endorphin physicsBodies from the added RS body attributes:\n"\
  "  BoneAxisIndex,AlignAxisIndex and WorldAlignAxisIndex"
  #Bodies are orientated 
  # 1)with a prescribed body axis (BoneAxisIndex) down the bone(parent joint to child joint)
  # 2)with a prescribed body axis (AlignAxisIndex) aligned to a prescribed world axis (WorldAlignAxisIndex)
  transform = nmx.Transform(physicsBody.parent())                            
  worldFrame = nmx.Matrix() #gives identity
  worldAxes = (worldFrame.xAxis(),worldFrame.yAxis(),worldFrame.zAxis())
  rockstarName = endorphinToRockstarName(physicsBody.Name)
  
  #Get alignment from endorphinDefault
  BoneAxisIndex = physicsBody.BoneAxisIndex.integer()
  BoneSign = math.copysign(1, BoneAxisIndex)
  BoneAxisIndex = abs(BoneAxisIndex) - 1#Convert x,y,z = 1,2,3 to 0,1,2
  
  AlignAxisIndex = physicsBody.AlignAxisIndex.integer()
  AlignSign = math.copysign(1, AlignAxisIndex)
  AlignAxisIndex = abs(AlignAxisIndex) - 1
  
  WorldAlignAxisIndex = physicsBody.WorldAlignAxisIndex.integer()
  AlignSign *= math.copysign(1, WorldAlignAxisIndex)
  WorldAlignAxisIndex = abs(WorldAlignAxisIndex) - 1
   
  OtherAxisIndex = (BoneAxisIndex + 1) % 3
  if OtherAxisIndex == AlignAxisIndex:
    OtherAxisIndex = (OtherAxisIndex + 1) % 3
    
  World_AlignedAxis = worldAxes[WorldAlignAxisIndex]

  pJoint = transform.parent()
  print "Body: ", physicsBody.Name
  print "Parent: ", pJoint.Name
  parentJointPos = pJoint.WorldMatrix().translation()
  #find child joint
  childObjects = pJoint.childObjects()
  for child in childObjects:
    if child.isType("PhysicsJoint"):
      break
  if not child.isType("PhysicsJoint"):
    print "******Assuming this is a hand or a foot and taking the child to be the bodyTM"
  print "Child: ", child.Name
  print " : "
 
  cJoint = child
  childJointPos = cJoint.WorldMatrix().translation()
  Body_BoneAxis = childJointPos.subtract(parentJointPos)#BodyAxes[BoneAxis].scale(BoneSign)
  Body_BoneAxis.normalize()
  Body_BoneAxis.scale(BoneSign)

  Body_AlignedAxis = nmx.Vector3()
  Body_AlignedAxis.crossProduct(Body_BoneAxis, World_AlignedAxis)
  Body_AlignedAxis.crossProduct(Body_BoneAxis)
  Body_AlignedAxis.normalize()
  Body_AlignedAxis.scale(AlignSign)
  
  Body_OtherAxis = nmx.Vector3()
  Body_OtherAxis.crossProduct(Body_BoneAxis, Body_AlignedAxis)
  Body_OtherAxis.normalize()
  if ((BoneAxisIndex + 1) % 3) != AlignAxisIndex:
    Body_OtherAxis.scale(-1.0)
  
  bodyAxes = {BoneAxisIndex : Body_BoneAxis}
  bodyAxes[AlignAxisIndex] = Body_AlignedAxis
  bodyAxes[OtherAxisIndex] = Body_OtherAxis
  worldFrame.setXAxis(bodyAxes[0])
  worldFrame.setYAxis(bodyAxes[1])
  worldFrame.setZAxis(bodyAxes[2])
  worldFrame.setTranslation(transform.worldTranslation())
  transform.setWorldMatrix(worldFrame)


#--------------------------------------------------------------------------------------------
def orientatePhysicsBodiesFromDefault(object):
  "orientate all endorphin physicsBodies from the added RS body attributes:\n"\
  "  BoneAxisIndex,AlignAxisIndex and WorldAlignAxisIndex"    
  childObjects = object.childObjects()
  if len(childObjects) > 0:
    for childObject in childObjects:
      if childObject.isType('PhysicsBody'):
        orientatePhysicsBodyFromDefault(childObject)
      orientatePhysicsBodiesFromDefault(childObject)

#--------------------------------------------------------------------------------------------
def orientateJointMovingFrameFromDefault(physicsJointLimit):
  "orientate endorphin physicsJoints and joints moving frame from the added RS Joint attributes:\n"\
  "  BoneAxisIndex,AlignAxisIndex and WorldAlignAxisIndex"
  #Joints are orientated 
  # 1)with a prescribed axis down the bone(parent joint to child joint)
  # 2)with a prescribed body axis aligned to a prescribed world axis 
  physicsJoint = nmx.PhysicsJointLimit(physicsJointLimit).physicsJoint()                            
  transform = physicsJointLimit.WorldMatrix()
  
  worldFrame = nmx.Matrix() #gives identity
  worldAxes = (worldFrame.xAxis(),worldFrame.yAxis(),worldFrame.zAxis())
  rockstarName = endorphinToRockstarName(physicsJointLimit.Name)
  
  #Get alignment from endorphinDefault
  BoneAxisIndex = physicsJointLimit.BoneAxisIndex.integer()
  BoneSign = math.copysign(1, BoneAxisIndex)
  BoneAxisIndex = abs(BoneAxisIndex) - 1#Convert x,y,z = 1,2,3 to 0,1,2
  
  AlignAxisIndex = physicsJointLimit.AlignAxisIndex.integer()
  AlignSign = math.copysign(1, AlignAxisIndex)
  AlignAxisIndex = abs(AlignAxisIndex) - 1
  
  WorldAlignAxisIndex = physicsJointLimit.WorldAlignAxisIndex.integer()
  AlignSign *= math.copysign(1, WorldAlignAxisIndex)
  WorldAlignAxisIndex = abs(WorldAlignAxisIndex) - 1
   
  OtherAxisIndex = (BoneAxisIndex + 1) % 3
  if OtherAxisIndex == AlignAxisIndex:
    OtherAxisIndex = (OtherAxisIndex + 1) % 3
    
  World_AlignedAxis = worldAxes[WorldAlignAxisIndex]

  pJoint = physicsJoint
  print "Joint: ", pJoint.Name
  parentJointPos = pJoint.WorldMatrix().translation()
  #find child joint
  childObjects = pJoint.childObjects()
  for child in childObjects:
    if child.isType("PhysicsJoint"):
      break
  if not child.isType("PhysicsJoint"):
    print "******Assuming this ia a hand or a foot and taking the child to be the bodyTM"
  print "Child: ", child.Name
  print " : "
 
  cJoint = child
  childJointPos = cJoint.WorldMatrix().translation()
  Body_BoneAxis = childJointPos.subtract(parentJointPos)#BodyAxes[BoneAxis].scale(BoneSign)
  Body_BoneAxis.normalize()
  Body_BoneAxis.scale(BoneSign)

  Body_AlignedAxis = nmx.Vector3()
  Body_AlignedAxis.crossProduct(Body_BoneAxis, World_AlignedAxis)
  Body_AlignedAxis.crossProduct(Body_BoneAxis)
  Body_AlignedAxis.normalize()
  Body_AlignedAxis.scale(AlignSign)
  
  Body_OtherAxis = nmx.Vector3()
  Body_OtherAxis.crossProduct(Body_BoneAxis, Body_AlignedAxis)
  Body_OtherAxis.normalize()
  if ((BoneAxisIndex + 1) % 3) != AlignAxisIndex:
    Body_OtherAxis.scale(-1.0)
  
  bodyAxes = {BoneAxisIndex : Body_BoneAxis}
  bodyAxes[AlignAxisIndex] = Body_AlignedAxis
  bodyAxes[OtherAxisIndex] = Body_OtherAxis
  worldFrame.setXAxis(bodyAxes[0])
  worldFrame.setYAxis(bodyAxes[1])
  worldFrame.setZAxis(bodyAxes[2])
  worldFrame.setTranslation(transform.translation())

  nmx.PhysicsJointLimit(physicsJointLimit).setWorldMatrix(worldFrame)

  #align moving frame with joint limit frame
  inverseWorldMatrix = physicsJoint.WorldMatrix()
  inverseWorldMatrix.invert()  
  worldFrame.multiply(inverseWorldMatrix)
  MovingOffsets = worldFrame.rotationEulerAngles()
  physicsJointLimit.attribute("MovingOffsetX").setFloat(MovingOffsets.x())
  physicsJointLimit.attribute("MovingOffsetY").setFloat(MovingOffsets.y())
  physicsJointLimit.attribute("MovingOffsetZ").setFloat(MovingOffsets.z())

  #fudge for North before I did it properly above 
  #  if physicsJointLimit.LimitType.string() == "BallSocket":
  #    physicsJointLimit.attribute("MovingOffsetX").setFloat(-0.5*Pi)
  #    physicsJointLimit.attribute("MovingOffsetY").setFloat(0.0)
  #    physicsJointLimit.attribute("MovingOffsetZ").setFloat(0.0)
  #  elif physicsJointLimit.LimitType.string() == "Hinge":
  #    physicsJointLimit.attribute("MovingOffsetX").setFloat(0.0)
  #    physicsJointLimit.attribute("MovingOffsetY").setFloat(-0.5*Pi)
  #    physicsJointLimit.attribute("MovingOffsetZ").setFloat(-0.5*Pi)

#unused
#--------------------------------------------------------------------------------------------
def orientateJointMovingFrame(physicsJointLimit, BoneAxisIndex, AlignAxisIndex, WorldAlignAxisIndex):
  "orientate endorphin physicsJoints and joints moving frame from the added RS Joint attributes:\n"\
  "  BoneAxisIndex,AlignAxisIndex and WorldAlignAxisIndex"
  #Joints are orientated 
  # 1)with a prescribed axis down the bone(parent joint to child joint)
  # 2)with a prescribed body axis aligned to a prescribed world axis 
  physicsJoint = nmx.PhysicsJointLimit(physicsJointLimit).physicsJoint()                            
  transform = physicsJointLimit.WorldMatrix()
  
  worldFrame = nmx.Matrix() #gives identity
  worldAxes = (worldFrame.xAxis(),worldFrame.yAxis(),worldFrame.zAxis())
  rockstarName = endorphinToRockstarName(physicsJointLimit.Name)
  
  #Get alignment from endorphinDefault
  #BoneAxisIndex = physicsJointLimit.BoneAxisIndex.integer()
  BoneSign = math.copysign(1, BoneAxisIndex)
  BoneAxisIndex = abs(BoneAxisIndex) - 1#Convert x,y,z = 1,2,3 to 0,1,2
  
  #AlignAxisIndex = physicsJointLimit.AlignAxisIndex.integer()
  AlignSign = math.copysign(1, AlignAxisIndex)
  AlignAxisIndex = abs(AlignAxisIndex) - 1
  
  #WorldAlignAxisIndex = physicsJointLimit.WorldAlignAxisIndex.integer()
  AlignSign *= math.copysign(1, WorldAlignAxisIndex)
  WorldAlignAxisIndex = abs(WorldAlignAxisIndex) - 1
   
  OtherAxisIndex = (BoneAxisIndex + 1) % 3
  if OtherAxisIndex == AlignAxisIndex:
    OtherAxisIndex = (OtherAxisIndex + 1) % 3
    
  World_AlignedAxis = worldAxes[WorldAlignAxisIndex]

  pJoint = physicsJoint
  print "Joint: ", pJoint.Name
  parentJointPos = pJoint.WorldMatrix().translation()
  #find child joint
  childObjects = pJoint.childObjects()
  for child in childObjects:
    if child.isType("PhysicsJoint"):
      break
  if not child.isType("PhysicsJoint"):
    print "******Assuming this ia a hand or a foot and taking the child to be the bodyTM"
  print "Child: ", child.Name
  print " : "
 
  cJoint = child
  childJointPos = cJoint.WorldMatrix().translation()
  Body_BoneAxis = childJointPos.subtract(parentJointPos)#BodyAxes[BoneAxis].scale(BoneSign)
  Body_BoneAxis.normalize()
  Body_BoneAxis.scale(BoneSign)

  Body_AlignedAxis = nmx.Vector3()
  Body_AlignedAxis.crossProduct(Body_BoneAxis, World_AlignedAxis)
  Body_AlignedAxis.crossProduct(Body_BoneAxis)
  Body_AlignedAxis.normalize()
  Body_AlignedAxis.scale(AlignSign)
  
  Body_OtherAxis = nmx.Vector3()
  Body_OtherAxis.crossProduct(Body_BoneAxis, Body_AlignedAxis)
  Body_OtherAxis.normalize()
  if ((BoneAxisIndex + 1) % 3) != AlignAxisIndex:
    Body_OtherAxis.scale(-1.0)
  
  bodyAxes = {BoneAxisIndex : Body_BoneAxis}
  bodyAxes[AlignAxisIndex] = Body_AlignedAxis
  bodyAxes[OtherAxisIndex] = Body_OtherAxis
  worldFrame.setXAxis(bodyAxes[0])
  worldFrame.setYAxis(bodyAxes[1])
  worldFrame.setZAxis(bodyAxes[2])
  worldFrame.setTranslation(transform.translation())

  nmx.PhysicsJointLimit(physicsJointLimit).setWorldMatrix(worldFrame)

  #align moving frame with joint limit frame
  inverseWorldMatrix = physicsJoint.WorldMatrix()
  inverseWorldMatrix.invert()  
  worldFrame.multiply(inverseWorldMatrix)
  MovingOffsets = worldFrame.rotationEulerAngles()
  physicsJointLimit.attribute("MovingOffsetX").setFloat(MovingOffsets.x())
  physicsJointLimit.attribute("MovingOffsetY").setFloat(MovingOffsets.y())
  physicsJointLimit.attribute("MovingOffsetZ").setFloat(MovingOffsets.z())

  #fudge for North before I did it properly above 
  #  if physicsJointLimit.LimitType.string() == "BallSocket":
  #    physicsJointLimit.attribute("MovingOffsetX").setFloat(-0.5*Pi)
  #    physicsJointLimit.attribute("MovingOffsetY").setFloat(0.0)
  #    physicsJointLimit.attribute("MovingOffsetZ").setFloat(0.0)
  #  elif physicsJointLimit.LimitType.string() == "Hinge":
  #    physicsJointLimit.attribute("MovingOffsetX").setFloat(0.0)
  #    physicsJointLimit.attribute("MovingOffsetY").setFloat(-0.5*Pi)
  #    physicsJointLimit.attribute("MovingOffsetZ").setFloat(-0.5*Pi)

#unused
#--------------------------------------------------------------------------------------------
def orientateJointFrameFromDefault(physicsJoint, BoneAxisIndex, AlignAxisIndex, WorldAlignAxisIndex):
  "orientate endorphin physicsJoints from the added RS Joint attributes:\n"\
  "  BoneAxisIndex,AlignAxisIndex and WorldAlignAxisIndex"
  #Joints are orientated 
  # 1)with a prescribed axis down the bone(parent joint to child joint)
  # 2)with a prescribed body axis aligned to a prescribed world axis 
  #physicsJoint = nmx.PhysicsJointLimit(physicsJointLimit).physicsJoint()                            
  transform = physicsJoint.WorldMatrix()
  
  worldFrame = nmx.Matrix() #gives identity
  worldAxes = (worldFrame.xAxis(),worldFrame.yAxis(),worldFrame.zAxis())
  
  #Get alignment from endorphinDefault
  #BoneAxisIndex = physicsJointLimit.BoneAxisIndex.integer()
  BoneSign = math.copysign(1, BoneAxisIndex)
  BoneAxisIndex = abs(BoneAxisIndex) - 1#Convert x,y,z = 1,2,3 to 0,1,2
  
  #AlignAxisIndex = physicsJointLimit.AlignAxisIndex.integer()
  AlignSign = math.copysign(1, AlignAxisIndex)
  AlignAxisIndex = abs(AlignAxisIndex) - 1
  
  #WorldAlignAxisIndex = physicsJointLimit.WorldAlignAxisIndex.integer()
  AlignSign *= math.copysign(1, WorldAlignAxisIndex)
  WorldAlignAxisIndex = abs(WorldAlignAxisIndex) - 1
   
  OtherAxisIndex = (BoneAxisIndex + 1) % 3
  if OtherAxisIndex == AlignAxisIndex:
    OtherAxisIndex = (OtherAxisIndex + 1) % 3
    
  World_AlignedAxis = worldAxes[WorldAlignAxisIndex]

  pJoint = physicsJoint
  print "Joint: ", pJoint.Name
  parentJointPos = pJoint.WorldMatrix().translation()
  #find child joint
  childObjects = pJoint.childObjects()
  for child in childObjects:
    if child.isType("PhysicsJoint"):
      break
  if not child.isType("PhysicsJoint"):
    print "******Assuming this ia a hand or a foot and taking the child to be the bodyTM"
  print "Child: ", child.Name
  print " : "
 
  cJoint = child
  childJointPos = cJoint.WorldMatrix().translation()
  Body_BoneAxis = childJointPos.subtract(parentJointPos)#BodyAxes[BoneAxis].scale(BoneSign)
  Body_BoneAxis.normalize()
  Body_BoneAxis.scale(BoneSign)

  Body_AlignedAxis = nmx.Vector3()
  Body_AlignedAxis.crossProduct(Body_BoneAxis, World_AlignedAxis)
  Body_AlignedAxis.crossProduct(Body_BoneAxis)
  Body_AlignedAxis.normalize()
  Body_AlignedAxis.scale(AlignSign)
  
  Body_OtherAxis = nmx.Vector3()
  Body_OtherAxis.crossProduct(Body_BoneAxis, Body_AlignedAxis)
  Body_OtherAxis.normalize()
  if ((BoneAxisIndex + 1) % 3) != AlignAxisIndex:
    Body_OtherAxis.scale(-1.0)
  
  bodyAxes = {BoneAxisIndex : Body_BoneAxis}
  bodyAxes[AlignAxisIndex] = Body_AlignedAxis
  bodyAxes[OtherAxisIndex] = Body_OtherAxis
  worldFrame.setXAxis(bodyAxes[0])
  worldFrame.setYAxis(bodyAxes[1])
  worldFrame.setZAxis(bodyAxes[2])
  worldFrame.setTranslation(transform.translation())

  nmx.PhysicsJoint(physicsJoint).setWorldMatrix(worldFrame)

#--------------------------------------------------------------------------------------------
def orientateJointMovingFramesFromDefault(object):
  "orientate all endorphin physicsJoints and joints moving frame from the added RS Joint attributes:\n"\
  "  BoneAxisIndex,AlignAxisIndex and WorldAlignAxisIndex"
  childObjects = object.childObjects()
  if len(childObjects) > 0:
    for childObject in childObjects:
      if childObject.isType('PhysicsJointLimit'):
        orientateJointMovingFrameFromDefault(childObject)
      orientateJointMovingFramesFromDefault(childObject)

overwrite = True
#############################################################################################
## Functions for saving modifications to the model in the defaults files  
#############################################################################################
#--------------------------------------------------------------------------------------------
def saveModifications(object):
  fileName = physicsCharacter.OutputDirectory.string() + physicsCharacter.AssetName.string() + "_Defaults.py"
  if overwrite == False and os.path.isfile(fileName) == True :
    print 'WARNING : the file already exists, it can\'t be overwritten'
  else :
    outputFile = open(fileName, "w")
    saveCharacterDefaults(outputFile, physicsCharacter)
    physicsCharacterRoot =  physicsCharacter.childObjects()[0].childObjects()[0]
    print "physicsCharacterRoot: ", physicsCharacterRoot.Name.string()
    #write root info 1st 
    jointChildObjects = physicsCharacterRoot.childObjects()
    bodyTransformName = physicsCharacterRoot.Name.string() +"PhysicsBodyTM"
    if len(jointChildObjects) > 0:
      for jointChildObject in jointChildObjects:
        print "jointChildObject: ", jointChildObject.Name.string()
        if jointChildObject.isType('Transform') and bodyTransformName == jointChildObject.Name.string():
          print jointChildObject.Name.string()
          outputFile.write('#--------------------------------------------------------------------------------------------\n')
          outputFile.write('#modified values for endorphin attributes that are RS attributes\n')
          outputFile.write('#--------------------------------------------------------------------------------------------\n')
          outputFile.write('#endorphin body modified values\n')
          outputFile.write('\n')
          saveBodyEndorphinDefaultsTry(outputFile, jointChildObject)
    recursiveSaveCharacterEndorphinDefaults(outputFile, physicsCharacterRoot)
    outputFile.close()

  #--------------------------------------------------------------------------------------------
def saveCharacterDefaults(outputFile, physicsCharacter):
  indent = '  '
  outputFile.write('#--------------------------------------------------------------------------------------------\n')
  outputFile.write('#modified values for added RS attributes\n')
  outputFile.write('#--------------------------------------------------------------------------------------------\n')
  outputFile.write('\n')
  saveRootDefaults(outputFile, physicsCharacter)  
  physicsCharacterPastRoot =  physicsCharacter.childObjects()[0].childObjects()[0]
  print "root name " + physicsCharacterPastRoot.Name()
  recursiveSaveCharacterDefaults(outputFile, physicsCharacterPastRoot)    

#--------------------------------------------------------------------------------------------
def savePhysicsJointLimitDefaults(physicsJointLimit,outputFile):
  indent =  '  '
  rockstarName = physicsJointLimit.Name.string()
  endName = endorphinToRockstarName(rockstarName)
  outputFile.write('default ["' + endName+'"] = {\n')
  physicsJoint = nmx.PhysicsJointLimit(physicsJointLimit).physicsJoint()                            
  if physicsJointLimit.LimitType.string() == "BallSocket":
    jointType = "threedof"
  elif physicsJointLimit.LimitType.string() == "Hinge":
    jointType = "onedof"
  print jointType   
  if jointType == "unknown": 
    print "Unkown joint type"
  if jointType == "threedof":
    label = indent + '"Limitenabled" : '
    outputFile.write(indent +'"BoneAxisIndex" : %s,\n' % physicsJointLimit.BoneAxisIndex.integer())
    outputFile.write(indent +'"AlignAxisIndex" : %s,\n' % physicsJointLimit.AlignAxisIndex.integer())
    outputFile.write(indent +'"WorldAlignAxisIndex" : %s,\n' % physicsJointLimit.WorldAlignAxisIndex.integer())
    writeBool(outputFile,label,physicsJointLimit.Limitenabled.boolean())
    outputFile.write(indent + '"Minfirstleanangle" : %s'% physicsJointLimit.Minfirstleanangle.float() + ',\n')
    outputFile.write(indent + '"Maxfirstleanangle" : %s'% physicsJointLimit.Maxfirstleanangle.float() + ',\n')
    outputFile.write(indent + '"Minsecondleanangle" : %s'% physicsJointLimit.Minsecondleanangle.float() + ',\n')
    outputFile.write(indent + '"Maxsecondleanangle" : %s'% physicsJointLimit.Maxsecondleanangle.float() + ',\n')
    outputFile.write(indent + '"Mintwistangle" : %s'% physicsJointLimit.Mintwistangle.float() + ',\n')
    outputFile.write(indent + '"Maxtwistangle" : %s'% physicsJointLimit.Maxtwistangle.float() + ',\n')
    label = indent + '"Createeffector" : '
    writeBool(outputFile,label,physicsJointLimit.Createeffector.boolean())
    label = indent + '"Reversefirstleanmotor" : '
    writeBool(outputFile,label,physicsJointLimit.Reversefirstleanmotor.boolean())
    outputFile.write(indent + '#if it is a right limb then reverse the lean2 and twist motors\n')
    label = indent + '"Reversesecondleanmotor" : '
    writeBool(outputFile,label,physicsJointLimit.Reversesecondleanmotor.boolean())
    label = indent + '"Reversetwistmotor" : '
    writeBool(outputFile,label,physicsJointLimit.Reversetwistmotor.boolean())
    outputFile.write(indent + '"Softlimitfirstleanmultiplier" : %s'% physicsJointLimit.Softlimitfirstleanmultiplier.float() + ',\n')
    outputFile.write(indent + '"Softlimitsecondleanmultiplier" : %s'% physicsJointLimit.Softlimitsecondleanmultiplier.float() + ',\n')
    outputFile.write(indent + '"Softlimittwistmultiplier" : %s'% physicsJointLimit.Softlimittwistmultiplier.float() + ',\n')
    outputFile.write(indent + '"Defaultleanforcecap" : %s'% physicsJointLimit.Defaultleanforcecap.float() + ',\n')
    outputFile.write(indent + '"Defaulttwistforcecap" : %s'% physicsJointLimit.Defaulttwistforcecap.float() + ',\n')
    outputFile.write(indent + '"Defaultmusclestiffness" : %s'% physicsJointLimit.Defaultmusclestiffness.float() + ',\n')
    outputFile.write(indent + '"Defaultmusclestrength" : %s'% physicsJointLimit.Defaultmusclestrength.float() + ',\n')
    outputFile.write(indent + '"Defaultmuscledamping" : %s'% physicsJointLimit.Defaultmuscledamping.float() + '}\n')
  elif jointType == "onedof":    
    outputFile.write(indent +'"BoneAxisIndex" : %s,\n' % physicsJointLimit.BoneAxisIndex.integer())
    outputFile.write(indent +'"AlignAxisIndex" : %s,\n' % physicsJointLimit.AlignAxisIndex.integer())
    outputFile.write(indent +'"WorldAlignAxisIndex" : %s,\n' % physicsJointLimit.WorldAlignAxisIndex.integer())
    label = indent + '"Limitenabled" : '
    writeBool(outputFile,label,physicsJointLimit.Limitenabled.boolean())        
    #NB. Rotating the joint limit around the hinge axis has no effect (in endorphin either)
    #moving frame can be outside of limit offset - mmmmtodo WARN here as why would you want to do that?
    max = 0.5*physicsJointLimit.TwistAngle.float() + physicsJointLimit.TwistOffset.float()
    min = physicsJointLimit.TwistOffset.float() - 0.5*physicsJointLimit.TwistAngle.float()
    outputFile.write(indent + '"Minangle" : %s'% min + ',\n')
    outputFile.write(indent + '"Maxangle" : %s'% max + ',\n')
    label = indent + '"Createeffector" : '
    writeBool(outputFile,label,physicsJointLimit.Createeffector.boolean())
    outputFile.write(indent + '"Defaultleanforcecap" : %s'% physicsJointLimit.Defaultleanforcecap.float() + ',\n')
    outputFile.write(indent + '"Defaultmusclestiffness" : %s'% physicsJointLimit.Defaultmusclestiffness.float() + ',\n')
    outputFile.write(indent + '"Defaultmusclestrength" : %s'% physicsJointLimit.Defaultmusclestrength.float() + ',\n')
    outputFile.write(indent + '"Defaultmuscledamping" : %s'% physicsJointLimit.Defaultmuscledamping.float() + '}\n')


#--------------------------------------------------------------------------------------------
def saveBodyEndorphinDefaultsTry(outputFile, physicsBody):
  indent = '  '
  childObjects = physicsBody.childObjects()
  if len(childObjects) > 0:
    for childObject in childObjects:
      if childObject.isType('PhysicsBody'):
        saveBodyEndorphinDefaults(outputFile, nmx.PhysicsBody(nmx.PhysicsBody(childObject)))
      else:
        print "ERROR: Root not a body, or Transform/PhysicsBodyTM doesn't have a child that is a PhysicsBody"

#--------------------------------------------------------------------------------------------
def recursiveSaveCharacterEndorphinDefaults(outputFile, object):
  indent = '  '
  print "_" + object.Name.string()
  childObjects = object.childObjects()
  if len(childObjects) > 0:
    for childObject in childObjects:
      print indent + " child " + childObject.Name.string()
      jointObject = childObject;
      jointLimitName = childObject.Name.string() + "PhysicsJointLimit"
      for childObject in childObjects:
        if childObject.isType('PhysicsJointLimit') and jointLimitName == childObject.Name.string():
          saveJointEndorphinDefaults(outputFile, childObject)
          jointChildObjects = jointObject.childObjects()
          print "_" + object.Name.string()
          for jointChildObject in jointChildObjects:
            bodyObject = jointChildObject;
            if jointChildObject.isType('Transform'):
              print jointChildObject.Name.string()
              saveBodyEndorphinDefaultsTry(outputFile, jointChildObject)
              recursiveSaveCharacterEndorphinDefaults(outputFile, jointObject)

#--------------------------------------------------------------------------------------------
def saveBodyEndorphinDefaults(outputFile, physicsBody):
  indent = '  '
  print("Writing Animation Bound Data %s" % endorphinToRockstarName(physicsBody.Name))
  outputFile.write('endorphinDefault["' + endorphinToRockstarName(physicsBody.Name) + '"] = {\n') 
  outputFile.write(indent +'"StaticFriction" : %s,\n' % physicsBody.StaticFriction)
  outputFile.write(indent +'"Restitution" : %s,\n' % physicsBody.Restitution)
  if physicsBody.Shape.string() == "Sphere":
    outputFile.write(indent +'"Shape" :"%s",\n' % physicsBody.Shape.string())  
    outputFile.write(indent +'"Radius" : %s}\n' % physicsBody.Radius) 
  elif physicsBody.Shape.string() == "Capsule":
    outputFile.write(indent +'"Shape" :"%s",\n' % physicsBody.Shape.string())  
    outputFile.write(indent +'"Radius" : %s,\n' % physicsBody.Radius) 
    outputFile.write(indent +'"Length" : %s}\n' % physicsBody.Length)
  elif physicsBody.Shape.string() == "Box":
    outputFile.write(indent +'"Shape" :"%s",\n' % physicsBody.Shape.string()) 
    outputFile.write(indent +'"Height" : %s,\n' % physicsBody.Height)  
    outputFile.write(indent +'"Length" : %s,\n' % physicsBody.Length)
    outputFile.write(indent +'"Width" : %s}\n' % physicsBody.Width)  

#--------------------------------------------------------------------------------------------
def saveJointEndorphinDefaults(outputFile, jointLimit):
  indent = '  '
  print("Writing Endorphin default Joint %s" % endorphinToRockstarName(jointLimit.Name))
  outputFile.write('endorphinDefault["' + endorphinToRockstarName(jointLimit.Name) + '"] = {\n') 
  outputFile.write(indent +'"LimitType" : "%s",\n' % jointLimit.LimitType.string())
  if jointLimit.LimitType.string() == "BallSocket":
    outputFile.write(indent +'"Swing1Angle" : %s,\n' % jointLimit.Swing1Angle)  
    outputFile.write(indent +'"Swing2Angle" : %s,\n' % jointLimit.Swing2Angle) 
  outputFile.write(indent +'"TwistAngle" : %s,\n' % jointLimit.TwistAngle)  
  outputFile.write(indent +'"TwistOffset" : %s}\n' % jointLimit.TwistOffset) 

#--------------------------------------------------------------------------------------------
def saveRootDefaults(outputFile, object):
  indent = '  '
  childObjects = object.childObjects()[0].childObjects()[0].childObjects()
  if len(childObjects) > 0:
    object = childObjects[0]
    for childObject in childObjects:
      if childObject.isType('Transform'):
        print childObject.Name
        saveArticulatedPartDefaults(outputFile, childObject)

#--------------------------------------------------------------------------------------------
def recursiveSaveCharacterDefaults(outputFile, object):
  print "_" + object.Name.string()
  childObjects = object.childObjects()
  if len(childObjects) > 0:
    indent = '  '
    for childObject in childObjects:
      print indent + " child " + childObject.Name.string()
      jointObject = childObject;
      jointLimitName = childObject.Name.string() + "PhysicsJointLimit"      
      for childObject in childObjects:
        if childObject.isType('PhysicsJointLimit') and jointLimitName == childObject.Name.string():
          print indent + " CHILD " + childObject.Name.string()
          print indent + "   joint " + childObject.Name.string()
          savePhysicsJointLimitDefaults(nmx.PhysicsJointLimit(nmx.PhysicsJointLimit(childObject)),outputFile)
          jointType = "unknown"
          if nmx.PhysicsJointLimit(nmx.PhysicsJointLimit(childObject)).LimitType.string() == "BallSocket":
            jointType = "threedof"
          elif nmx.PhysicsJointLimit(nmx.PhysicsJointLimit(childObject)).LimitType.string() == "Hinge":
            jointType = "onedof"
          jointChildObjects = jointObject.childObjects()
          for jointChildObject in jointChildObjects:
            bodyObject = jointChildObject;
            print indent + "     jointChildObject " + jointChildObject.Name.string()
            if jointChildObject.isType('Transform'):
              print indent + "   JOINTCHILDOBJECT " + jointChildObject.Name.string()
              saveArticulatedPartDefaults(outputFile, jointChildObject)
              recursiveSaveCharacterDefaults(outputFile, jointObject)

#--------------------------------------------------------------------------------------------
              
def saveArticulatedPartDefaults(outputFile, object):

  childObjects = object.childObjects()
  if len(childObjects) > 0:
    for childObject in childObjects:
      if childObject.isType('PhysicsBody'):
        savePhysicsBodyDefaults(outputFile, nmx.PhysicsBody(nmx.PhysicsBody(childObject)))
      else:
        print "ERROR: Root not a body, or Transform/PhysicsBodyTM doesn't have a child that is a PhysicsBody"



#--------------------------------------------------------------------------------------------
def savePhysicsBodyDefaults(outputFile, physicsBody):
  indent = '  '
  #articulated Body Part info
  outputFile.write('default["%s"] = {\n' % endorphinToRockstarName(physicsBody.Name))
  label = indent + '"Hascollision" : '
  outputFile.write(indent +'"BoneAxisIndex" : %s,\n' % physicsBody.BoneAxisIndex.integer())
  outputFile.write(indent +'"AlignAxisIndex" : %s,\n' % physicsBody.AlignAxisIndex.integer())
  outputFile.write(indent +'"WorldAlignAxisIndex" : %s,\n' % physicsBody.WorldAlignAxisIndex.integer())
  writeBool(outputFile,label,physicsBody.Hascollision.boolean())
  outputFile.write(indent +'"CentreofmassX" : %s,\n' % physicsBody.CentreofmassX.float())
  outputFile.write(indent +'"CentreofmassY" : %s,\n' % physicsBody.CentreofmassY.float())
  outputFile.write(indent +'"CentreofmassZ" : %s,\n' % physicsBody.CentreofmassZ.float())
  outputFile.write(indent +'"Rs_mass" : %s,\n' % physicsBody.Rs_mass)
  outputFile.write(indent +'"Lineardamping" : %s,\n' % physicsBody.Lineardamping.float())
  outputFile.write(indent +'"Angulardamping" : %s,\n' % physicsBody.Angulardamping.float())
  outputFile.write(indent +'"Gravityscale" : %s,\n' % physicsBody.Gravityscale.float())
  outputFile.write(indent +'"Inertiamultiplier" : %s}\n' % physicsBody.Inertiamultiplier.float())
  
#--------------------------------------------------------------------------------------------
def writeBool(outputFile,label,value):
  print value
  if value == True:
    outputFile.write(label + 'True,\n')
  else:
    outputFile.write(label + 'False,\n')


#--------------------------------------------------------------------------------------------
def initWorkspace(physicsCharacter):
  "Adds filename attributes to the physicsCharacter and \n"\
  "fills them with default file paths and file names"
  physicsCharacter.addStringAttribute("AssetName", r"NM_Character", False)
  physicsCharacter.addIntegerAttribute("AssetTypeId", 0, 0, 50, False)
  physicsCharacter.addStringAttribute("InputDirectory", r"C:/", False)
  physicsCharacter.addStringAttribute("OutputDirectory", r"C:/", False)
  
  #Set character->IsEditable to On
  nmx.scene().object("|Scene|character").attribute("IsEditable").setBoolean(True)
  nmx.scene().object("|Scene|character").addFloatAttribute("Trans2ModelX",0.0,-360.0,360.0,False)
  nmx.scene().object("|Scene|character").addFloatAttribute("Trans2ModelY",0.0,-360.0,360.0,False)
  nmx.scene().object("|Scene|character").addFloatAttribute("Trans2ModelZ",0.0,-1000.0,1000.0,False)
  nmx.scene().object("|Scene|character").addFloatAttribute("Rotate2ModelX",0.0,-360.0,360.0,False)
  nmx.scene().object("|Scene|character").addFloatAttribute("Rotate2ModelY",0.0,-360.0,360.0,False)
  nmx.scene().object("|Scene|character").addFloatAttribute("Rotate2ModelZ",0.0,-360.0,360.0,False)

  #Add ChangeCapsuleLength and set to false
  #nmx.scene().object("|Scene|character|physicsCharacter").addBooleanAttribute("ChangeCapsuleLength",False,False)

 
#--------------------------------------------------------------------------------------------
def transformCharacter2Model(physicsCharacter, undo):
  "Rotates the character by angle Rotate2ModelX,Rotate2ModelY,Rotate2ModelZ"
  mult = 1.0
  if (undo):
    mult = -1.0
  angleX = mult * physicsCharacter.parent().attribute("Rotate2ModelX").double()*Pi/180.0
  angleY = mult * physicsCharacter.parent().attribute("Rotate2ModelY").double()*Pi/180.0
  angleZ = mult * physicsCharacter.parent().attribute("Rotate2ModelZ").double()*Pi/180.0
  transX = mult * physicsCharacter.parent().attribute("Trans2ModelX").double()
  transY = mult * physicsCharacter.parent().attribute("Trans2ModelY").double()
  transZ = mult * physicsCharacter.parent().attribute("Trans2ModelZ").double()
  #RotateX
  attribute = physicsCharacter.parent().attribute("RotationX")
  setAngle = attribute.double()+angleX
  if (setAngle > 2.0 * Pi):
    setAngle = setAngle - (2.0*Pi)
  if (setAngle < -2.0 * Pi):
    setAngle = setAngle + (2.0*Pi)
  attribute.setFloat(setAngle)
  #RotateY
  attribute = physicsCharacter.parent().attribute("RotationY")
  setAngle = attribute.double()+angleY
  if (setAngle > 2.0 * Pi):
    setAngle = setAngle - (2.0*Pi)
  if (setAngle < -2.0 * Pi):
    setAngle = setAngle + (2.0*Pi)
  attribute.setFloat(setAngle)
  #RotateZ
  attribute = physicsCharacter.parent().attribute("RotationZ")
  setAngle = attribute.double()+angleZ
  if (setAngle > 2.0 * Pi):
    setAngle = setAngle - (2.0*Pi)
  if (setAngle < -2.0 * Pi):
    setAngle = setAngle + (2.0*Pi)
  attribute.setFloat(setAngle)
  #TransX
  attribute = physicsCharacter.parent().attribute("TranslationX")
  attribute.setFloat(attribute.double()+transX)
  #TransY
  attribute = physicsCharacter.parent().attribute("TranslationY")
  attribute.setFloat(attribute.double()+transY)
  #TransZ
  attribute = physicsCharacter.parent().attribute("TranslationZ")
  attribute.setFloat(attribute.double()+transZ)


#--------------------------------------------------------------------------------------------
def removeJoint():
  "Remove a selected joint (whether it has children or not)\n"\
  " and keep its children attached to the removed joints parent.\n"\
  " NB: May have unstable results if you delete the root joint"

  if (nmx.scene().Selection.count() == 1):
    removeJoint = nmx.scene().Selection.object(0)
    if (removeJoint.isType('PhysicsJoint')):
      parentJoint = removeJoint.parent()#maybe physicCharacter if joint to be removed is the root
      childObjects = removeJoint.childObjects()
      for childObject in childObjects:
        print childObject.Name()
        if childObject.isType('PhysicsJointLimit'):
          childObject.unparent()
          childObject.setParent(parentJoint)
        if childObject.isType('PhysicsJoint'):
          childObject.unparent()
          childObject.setParent(parentJoint)
      removeJoint.unparent()
      removeJoint.delete()

    else:
      print ("ERROR: can only remove physics joints")
  else:
    print ("ERROR: no joint selected or more than one object selected for removal")
























