------------------------------------------------------------------------------------------------------------------------
-- Functions for writing out rockstar ragdoll data.
------------------------------------------------------------------------------------------------------------------------

------------------------------------------------------------------------------------------------------------------------
-- include other required scripts
------------------------------------------------------------------------------------------------------------------------
require "rockstar/RockstarUtilities.lua"


-- this rotation required to orient capsules down the y-axis instead of the z-axis
--
local xAxis = nmx.Vector3.new(-1, 0, 0)
local yAxis = nmx.Vector3.new(0, 0, -1)
local zAxis = nmx.Vector3.new(0, -1, 0)
local trans = nmx.Vector3.new(0, 0, 0)

local capsuleOrientationTransform = nmx.Matrix.new()
capsuleOrientationTransform:initialise(xAxis, yAxis, zAxis, trans)
--Next lines are equivalent to above
--local capsuleOrientationTransform2 = nmx.Matrix.new()
--capsuleOrientationTransform:set3x3ToXRotation(0.5 * math.pi)
--capsuleOrientationTransform2:set3x3ToYRotation(math.pi)
--capsuleOrientationTransform:multiply(capsuleOrientationTransform2)

------------------------------------------------------------------------------------------------------------------------
--| signature: nil writeEuphoriaPhysicsBody(
--|   table            exportWriter,
--|   integer          indent,
--|   sgTransformNode  physicsBodyTxInstance,
--|   PhysicsBodyNode  physicsBody,
--|   number           partNumber,
--|   Vector3          rootPos)
------------------------------------------------------------------------------------------------------------------------
local writeEuphoriaPhysicsBody = function(exportWriter, indent, physicsBodyTxInstance, physicsBody, partNumber, rootPos)
  -- assert all arguments are valid
  --
  assert(type(exportWriter) == "table", "bad argument #1 to 'rockstar.writeRagdollPhysicsBody'")
  assert(type(indent) == "number", "bad argument #2 to 'rockstar.writeRagdollPhysicsBody'")
  assert(type(physicsBodyTxInstance) == "userdata" and physicsBodyTxInstance:is(nmx.sgTransformNode.ClassTypeId()), "bad argument #3 to 'rockstar.writeRagdollPhysicsBody'")
  assert(type(physicsBody) == "userdata" and physicsBody:isDerivedFrom(nmx.PhysicsBodyNode.ClassTypeId()), "bad argument #4 to 'rockstar.writeRagdollPhysicsBody'")

  -- begin bodyPart tag
  --
  exportWriter:write(indent, "<bodyPart name=\"", physicsBodyTxInstance:getName(), "\">")
  indent = indent + 1

  exportWriter:write(indent, "<partNumber>", partNumber, "</partNumber>")
  
  local rockstarPhysicsData = rockstar.getRockstarPhysicsEngineData(physicsBody)
  --Writes: PartBouyancyMultiplier
  if rockstarPhysicsData then
    rockstar.writeCustomRockstarProperties(exportWriter, indent, rockstarPhysicsData)
  end

  exportWriter:write(indent, "<transform>")
  indent = indent + 1

  local physicsVolumeTxInstance, physicsVolume = rockstar.getPhysicsVolume(physicsBodyTxInstance, physicsBody)
  assert(physicsVolumeTxInstance and physicsVolume, string.format("PhysicsBodyNode '%s' has no associated PhysicsVolumeNode", physicsBodyTxInstance:getName()))

  local worldMatrix = physicsVolumeTxInstance:getWorldMatrix()

  local primitive = physicsVolume:getShape() 
  if primitive:is(nmx.CapsuleNode.ClassTypeId()) then
    local translation = nmx.Vector3.new(worldMatrix:translation())
    worldMatrix:transpose3x3()
    worldMatrix:multiply(capsuleOrientationTransform)
    worldMatrix:translation():set(translation)
  else
    worldMatrix:transpose3x3()
  end
  
  
  local xAxis = worldMatrix:xAxis()
  exportWriter:write(indent, "<r0 c0=\"", xAxis:getX(), "\" c1=\"", xAxis:getY(), "\" c2=\"", xAxis:getZ(), "\" c3=\"", 0, "\"/>")

  local yAxis = worldMatrix:yAxis()
  exportWriter:write(indent, "<r1 c0=\"", yAxis:getX(), "\" c1=\"", yAxis:getY(), "\" c2=\"", yAxis:getZ(), "\" c3=\"", 0, "\"/>")

  local zAxis = worldMatrix:zAxis()
  exportWriter:write(indent, "<r2 c0=\"", zAxis:getX(), "\" c1=\"", zAxis:getY(), "\" c2=\"", zAxis:getZ(), "\" c3=\"", 0, "\"/>")

  local translation = worldMatrix:translation()
  translation = worldMatrix:translation():subtract(rootPos)
  exportWriter:write(indent, "<r3 c0=\"", translation:getX(), "\" c1=\"", translation:getY(), "\" c2=\"", translation:getZ(), "\" c3=\"", 1, "\"/>")

  indent = indent - 1
  exportWriter:write(indent, "</transform>")


  -- end bodyPart tag
  --
  indent = indent - 1
  exportWriter:write(indent, "</bodyPart>")
end

------------------------------------------------------------------------------------------------------------------------
--| signature: nil writeRagdollPhysicsJointLimit(
--|   table                 exportWriter,
--|   integer               indent,
--|   sgTransformNode       physicsJointLimitTxInstance,
--|   PhysicsJointLimitNode physicsJointLimit
--|   integer               physicsJointIndex,
--|   integer               physicsJointParentIndex)
------------------------------------------------------------------------------------------------------------------------
local writeEuphoriaPhysicsJointLimit = function(exportWriter, indent, physicsJointLimitTxInstance, physicsJointLimit, physicsJointIndex, physicsJointParentIndex)
  -- assert all arguments are valid
  --
  assert(type(exportWriter) == "table", "bad argument #1 to 'rockstar.writeEuphoriaPhysicsJointLimit'")
  assert(type(indent) == "number", "bad argument #2 to 'rockstar.writeEuphoriaPhysicsJointLimit'")
  assert(type(physicsJointLimitTxInstance) == "userdata" and physicsJointLimitTxInstance:is(nmx.sgTransformNode.ClassTypeId()), "bad argument #3 to 'rockstar.writeEuphoriaPhysicsJointLimit'")
  assert(type(physicsJointLimit) == "userdata" and physicsJointLimit:isDerivedFrom(nmx.PhysicsJointLimitNode.ClassTypeId()), "bad argument #4 to 'rockstar.writeEuphoriaPhysicsJointLimit'")
  assert(type(physicsJointIndex) == "number", "bad argument #5 to 'rockstar.writeEuphoriaPhysicsJointLimit'")
  assert(type(physicsJointParentIndex) == "number", "bad argument #6 to 'rockstar.writeEuphoriaPhysicsJointLimit'")

  local rockstarPhysicsData = rockstar.getRockstarPhysicsEngineData(physicsJointLimit)
  assert(rockstarPhysicsData, string.format("PhysicsJointLimitNode '%s' is missing rockstar physics data", physicsJointLimitTxInstance:getName()))

  local effectorNameAttribute = rockstarPhysicsData:findAttribute("EffectorName")
  assert(effectorNameAttribute:isValid(), "Rockstar PhysicsJointLimit data is missing 'EffectorName' attribute")
  local effectorName = effectorNameAttribute:asString()

  --if physicsJointLimit:is(nmx.PhysicsHingeNode.ClassTypeId()) then
  --1dof joints are currently 3dof joints with swing1 and swing2 switched off
  --  The hinge joint in morpheme has different axes(?) and cannot be edited into the correct configuration
  if (physicsJointLimit:is(nmx.PhysicsTwistSwingNode.ClassTypeId()) and not (physicsJointLimit:findAttribute("Swing1Active"):asBool() or physicsJointLimit:findAttribute("Swing2Active"):asBool())) then
    -- begin PhysicsJointLimit tag
    --
    exportWriter:write(indent, "<onedof name=\"", effectorName, "\">")
    indent = indent + 1

    exportWriter:write(indent, "<jointNumber>", physicsJointIndex, "</jointNumber>")
    exportWriter:write(indent, "<parentIndex>", physicsJointParentIndex, "</parentIndex>")

    rockstar.writeHingeLimitAngleData(exportWriter, indent, physicsJointLimitTxInstance, physicsJointLimit)
    rockstar.writeCustomRockstarProperties(exportWriter, indent, rockstarPhysicsData)

    -- end PhysicsJointLimit tag
    --
    indent = indent - 1
    exportWriter:write(indent, "</onedof>")
  elseif physicsJointLimit:is(nmx.PhysicsTwistSwingNode.ClassTypeId()) then
    -- begin PhysicsJointLimit tag
    --
    exportWriter:write(indent, "<threedof name=\"", effectorName, "\">")
    indent = indent + 1

    exportWriter:write(indent, "<jointNumber>", physicsJointIndex, "</jointNumber>")
    exportWriter:write(indent, "<parentIndex>", physicsJointParentIndex, "</parentIndex>")

    rockstar.writeTwistSwingLimitAngleData(exportWriter, indent, physicsJointLimitTxInstance, physicsJointLimit)
    rockstar.writeCustomRockstarProperties(exportWriter, indent, rockstarPhysicsData)

    -- end PhysicsJointLimit tag
    --
    indent = indent - 1
    exportWriter:write(indent, "</threedof>")
  end
end

------------------------------------------------------------------------------------------------------------------------
--| signature: nil rockstar.writeEuphoriaRig(
--|   table            exportWriter,
--|   sgTransformNodes exportedPhysicsJointTxInstances)
------------------------------------------------------------------------------------------------------------------------
rockstar.writeEuphoriaRig = function(exportWriter, exportedPhysicsJointTxInstances)
  local rootPos = nmx.Vector3.new()
  -- make a reverse map of effector name to index
  local euphoriaEffectorIndexes = {}
  for i = 1, table.getn(rockstar.euphoriaEffectorNames) do
    local effectorName = rockstar.euphoriaEffectorNames[i]
    euphoriaEffectorIndexes[effectorName] = i
  end

  -- build up a map of euphoria effector names to joint limits
  local euphoriaEffectors = {}
  local euphoriaParts = {}

  -- loop through backwards so parents are done before children, the array is backwards due to the way the
  -- list of joints was built up via recursion
  --
  for i = table.getn(exportedPhysicsJointTxInstances), 1, -1 do
    local physicsJointTxInstance = exportedPhysicsJointTxInstances[i]
    local physicsJoint = nmx.PhysicsJointNode.getPhysicsJoint(physicsJointTxInstance)

    local physicsJointLimitTxInstance, physicsJointLimit = rockstar.getPhysicsJointLimit(physicsJointTxInstance, physicsJoint)
    if physicsJointLimitTxInstance and physicsJointLimit then
      local rockstarPhysicsData = rockstar.getRockstarPhysicsEngineData(physicsJointLimit)

      local euphoriaEffectorAttribute = rockstarPhysicsData:findAttribute("EffectorName")
      local euphoriaEffectorName = euphoriaEffectorAttribute:asString()

      assert(euphoriaEffectorIndexes[euphoriaEffectorName], string.format("euphoria effector '%s' is not a valid effector name", euphoriaEffectorName))
      assert(not euphoriaEffectors[euphoriaEffectorName], string.format("euphoria effector '%s' has already specified, all effectors must be unique", euphoriaEffectorName))

      euphoriaEffectors[euphoriaEffectorName] = {
        physicsJointTxInstance = physicsJointTxInstance,
        physicsJoint = physicsJoint,
        physicsJointLimitTxInstance = physicsJointLimitTxInstance,
        physicsJointLimit = physicsJointLimit,
      }

      local index = euphoriaEffectorIndexes[euphoriaEffectorName]
      local physicsBodyTxInstance, physicsBody = rockstar.getPhysicsBody(physicsJointTxInstance, physicsJoint)
      euphoriaParts[index + 1] = {
        physicsJointTxInstance = physicsJointTxInstance,
        physicsJoint = physicsJoint,
        physicsBodyTxInstance = physicsBodyTxInstance,
        physicsBody = physicsBody,
      }
      euphoriaParts[physicsJointTxInstance:getName()] = index + 1
    else
      -- root joint won't have a limit
      --
      assert(not euphoriaParts[1], "more than one joint with no joint limit encountered, there can be only one root joint")
      local physicsBodyTxInstance, physicsBody = rockstar.getPhysicsBody(physicsJointTxInstance, physicsJoint)
      euphoriaParts[1] = {
        physicsBodyTxInstance = physicsBodyTxInstance,
        physicsBody = physicsBody,
      }
      -- have to use a string index as using the userdata as a key doesn't work when retrieving the index
      --
      euphoriaParts[physicsJointTxInstance:getName()] = 1
      --get the root parts global translation - all output part translations are relative to this
      local rootPhysicsVolumeTxInstance, rootPhysicsVolume = rockstar.getPhysicsVolume(physicsBodyTxInstance, physicsBody)
      assert(rootPhysicsVolumeTxInstance and rootPhysicsVolume, string.format("PhysicsBodyNode '%s' has no associated PhysicsVolumeNode", rootPhysicsVolumeTxInstance:getName()))
      local worldMatrix = rootPhysicsVolumeTxInstance:getWorldMatrix()
      rootPos = worldMatrix:translation()
    end
  end

  local indent = 0
  exportWriter:write(indent, [[<?xml version="1.0" encoding="UTF-8"?>]])
  exportWriter:write(indent, [[<rockstar master_version="1" representation_version="0">]])
  indent = indent + 1

  exportWriter:write(indent, "<name>Fred</name>")
  exportWriter:write(indent, "<id>0</id>")

  local bodyBouyancyMultiplier = 97.0
  local dragMultiplier = 2.0
  local weightBeltMultiplier = 0.0

  exportWriter:write(indent, "<bodybouyancymultiplier>", bodyBouyancyMultiplier, "</bodybouyancymultiplier>")
  exportWriter:write(indent, "<dragmultiplier>", dragMultiplier, "</dragmultiplier>")
  exportWriter:write(indent, "<weightbeltmultiplier>", weightBeltMultiplier, "</weightbeltmultiplier>")

  -- export all the euphoria effector data
  --
  for i = 1, table.getn(rockstar.euphoriaEffectorNames) do
    local euphoriaEffectorName = rockstar.euphoriaEffectorNames[i]
    local euphoriaEffector = euphoriaEffectors[euphoriaEffectorName]
    assert(euphoriaEffector, string.format("euphoria effector '%s' was not found, all effectors must be specified", euphoriaEffectorName))

    -- the index is an index in to a lua array so adjust to make it a c-style array
    --
    local parentTxInstance = euphoriaEffector.physicsJointTxInstance:getParent()
    local parentIndex = euphoriaParts[parentTxInstance:getName()] - 1

    writeEuphoriaPhysicsJointLimit(
      exportWriter,
      indent,
      euphoriaEffector.physicsJointLimitTxInstance,
      euphoriaEffector.physicsJointLimit,
      i - 1,
      parentIndex)
  end
  
  -- export all the body part data
  --
  for i = 1, table.getn(euphoriaParts) do
    local euphoriaPart = euphoriaParts[i]

    writeEuphoriaPhysicsBody(
      exportWriter,
      indent,
      euphoriaPart.physicsBodyTxInstance,
      euphoriaPart.physicsBody,
      i - 1,
      rootPos)
  end

  indent = indent - 1
  exportWriter:write(indent, [[</rockstar>]])
end