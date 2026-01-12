#	Generates R* documentation from NmRsMessageDefinitions.h
#
#	Replaces old DocoMessages C program and is significantly
# more robust to extra lines and missing comments and the
# like.
#
# It's somewhat robust to #ifs so long as they are accounted
# for in the preprocessorFlags and preprocessorValues lists.
# Boolean combinations are *not* supported.  E.g. #if FLAG
# works but #if FLAG && ANOTHER_FLAG does not.

import re
import sys

if len(sys.argv) != 4:
	print "# Generates R* documentation from NmRsMessageDefinitions.h"
	print "#"
	print "# Usage: docomessages source htmlOutput xmlOutput"
	print "#"
	print "#	source - path to NmRsMessageDefinitions.h"
	print "#	htmlOutput - output path for HTML documentation"
	print "#	xmlOutput - output path for XML manifest"
	print "#"
	print "# Replaces old DocoMessages C program and is significantly"
	print "# more robust to extra lines and missing comments and the"
	print "# like."
	print "#"
	print "# It's somewhat robust to #ifs so long as they are accounted"
	print "# for in the preprocessorFlags and preprocessorValues lists."
	print "# Boolean combinations are *not* supported.  E.g. #if FLAG"
	print "# works but #if FLAG && ANOTHER_FLAG does not."
	
else:
	
	inputFilePath = sys.argv[1]
	htmlFilePath = sys.argv[2]
	xmlFilePath = sys.argv[3]
	
	# Preprocessor flags (0 = clear, 1 = set)
	preprocessorFlags = [ "ALLOW_DEBUG_BEHAVIOURS", "ALLOW_TRAINING_BEHAVIOURS", "NM_EA", "ALLOW_BEHAVIOURS_UNNECESSARY_FOR_GTA_V"]
	preprocessorValues = [ 0, 0, 0, 0]
	count = 0
	for flag in preprocessorFlags:
		print "#define %s %d" % (flag, preprocessorValues[count])
		count = count + 1
	
	assetName = "Cowboy/Cowgirl/Fred/Wilma/MPLarge/MPMedium"
	
	# Define some fancy background coloring gfor HTML
	background = ["\t\t<td style=\"background-color: rgb(255, 255, 255);\"", "\t\t<td style=\"background-color: rgb(234, 234, 234);\""]
	feedbackBackground = ["\t\t<td style=\"background-color: rgb(234, 220, 210);\"", "\t\t<td style=\"background-color: rgb(255, 245, 240);\""]
	
	inFile = open(inputFilePath, 'r')
	outFileHTML = open(htmlFilePath, 'w')
	outFileXML = open(xmlFilePath, 'w')
	
	# HTML Header
	outFileHTML.write("<html>\n")
	outFileHTML.write("<head>\n")
	outFileHTML.write("<title>%s Asset</title>\n" % (assetName))
	outFileHTML.write("<style type=\"text/css\">table.stat th, table.stat td { font-size : 77%; font-family : \"Myriad Web\",Verdana,Helvetica,Arial,sans-serif;  background : #efe none; color : #630; }\n")
	outFileHTML.write("BODY  {FONT-FAMILY: Tahoma; FONT-SIZE: 10pt}\n")
	outFileHTML.write("H3 {FONT-SIZE: 14pt; COLOR:#000000}\n")
	outFileHTML.write("H2 {FONT-SIZE: 18pt; text-decoration: underline; text-align: center; font-weight: bold; COLOR:#000000}</style></head>\n")
	outFileHTML.write("<body>\n")
	outFileHTML.write("<h2>%s Asset</h2>\n" % (assetName))
	outFileHTML.write("<br><center><A name=\"quicklink\"><h2>Quick Links</h2></A>\n")
	outFileHTML.write("<br><table class=\"stat\" width=100% cellpadding=10>\n")
	outFileHTML.write("<tr><td valign =\"top\" align=\"center\">\n")
	outFileHTML.write("<table class=\"stat\">\n")
	outFileHTML.write("<tr><td width = 100%><b>All Messages</b></td></tr>\n")
	
	# Manifest Header
	outFileXML.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n")
	outFileXML.write("<rage__NMBehaviorPool>\n")
	outFileXML.write("\t<behaviors>\n")
		
	# Loop through intput file looking for behaviour definitions
	# Make a nice table of contents
	toggle = 0
	visible = 1
	depth = 0
	flagDepth = 0
	line = inFile.readline()
	while line != "":
		# Handle simple #ifs
		if line.find("#if") != -1:
			count = 0
			depth = depth + 1
			flagDepth = depth
			for flag in preprocessorFlags:
				if line.find(flag) != -1:
					if preprocessorValues[count] == 0:
						visible = 0
				count = count + 1
		if line.find("#endif") != -1:
			if depth == flagDepth and visible == 0:
				visible = 1
			depth = depth - 1
		# Make table of contents entry
		if line.find("BEHAVIOUR(") != -1 and visible == 1:
			start = line.find('(') + 1
			end = line.find(')')
			name = line[start:end]
			words = re.split('[()]', line)
			name = words[1]
			outFileHTML.write("\t<tr>\n")
			outFileHTML.write("%s width = 100%%><a href=\"#%s\">%s</a></td>\n" % (background[toggle], name, name))
			outFileHTML.write("\t</tr>\n")
			toggle = 1 - toggle
		line = inFile.readline()
	outFileHTML.write("</table></td>\n")
	outFileHTML.write("</tr></table></center>\n")
	
	# Some basic description
	outFileHTML.write("<b>UNITS</b><p>")
	outFileHTML.write("<pre>All basic measurements are in standard units (kilograms, metres, seconds, radians)\n")
	outFileHTML.write("All other parameters will, unless otherwise stated, use the following units:\n\n")
	outFileHTML.write("position vectors:\tworld space (in metres)\n")
	outFileHTML.write("stiffness:\t\tstandardised to 6=limp, 10=normal, 16=strong\n")
	outFileHTML.write("damping:\t\tstandardised to 0.5=underdamped, 1=critically damped, 2=overdamped\n")
	outFileHTML.write("mask:\t\t\t\"fb\"=full body\n")
	outFileHTML.write("\t\t\t\"ub\"=upper body, \"ua\"=upper arms, \"uc\"=clavicles, \"us\"=upper spine\n")
	outFileHTML.write("\t\t\t\"lb\"=lower body, \"ll\"=left leg, \"lr\"=right leg\n")
	outFileHTML.write("</pre>")
	
	# The behaviour messages themselves
	outFileHTML.write("<br><br><h2>The Behaviour Messages</h2><br><p>These are the behaviour messages.</p>\n")
	
	outFileHTML.write("<pre>bvid_buoyancy\n")
	outFileHTML.write("bvid_dynamicBalancer\n")
	outFileHTML.write("bvid_bodyBalance\n")
	outFileHTML.write("bvid_braceForImpact\n")
	outFileHTML.write("bvid_staggerFall\n")
	outFileHTML.write("bvid_learnedCrawl\n")
	outFileHTML.write("bvid_bodyFoetal\n")
	outFileHTML.write("bvid_shot\n")
	outFileHTML.write("bvid_teeter\n")
	outFileHTML.write("bvid_armsWindmill\n")
	outFileHTML.write("bvid_armsWindmillAdaptive\n")
	outFileHTML.write("bvid_balancerCollisionsReaction\n")
	outFileHTML.write("bvid_spineTwist\n")
	outFileHTML.write("bvid_catchFall\n")
	outFileHTML.write("bvid_yanked\n")
	outFileHTML.write("bvid_dragged\n")
	outFileHTML.write("bvid_bodyRollUp\n")
	outFileHTML.write("bvid_upperBodyFlinch\n")
	outFileHTML.write("bvid_fallOverWall\n")
	outFileHTML.write("bvid_highFall\n")
	outFileHTML.write("bvid_rollDownStairs\n")
	outFileHTML.write("bvid_pedalLegs\n")
	outFileHTML.write("bvid_stumble\n")
	outFileHTML.write("bvid_grab\n")
	outFileHTML.write("bvid_animPose\n")
	outFileHTML.write("bvid_bodyWrithe\n")
	outFileHTML.write("bvid_headLook\n")
	outFileHTML.write("bvid_pointArm\n")
	outFileHTML.write("bvid_pointGun\n")
	outFileHTML.write("bvid_electrocute\n")
	outFileHTML.write("bvid_quadDeath\n")
	outFileHTML.write("</pre>")

	# Loop through the BEHAVIOUR blocks
	inFile.seek(0) # start file again!
	line = inFile.readline()
	description = ""
	visible = 1
	depth = 0
	flagDepth = 0
	while line != "":
		
		# Handle simple #ifs
		if line.find("#if") != -1:
			count = 0
			depth = depth + 1
			flagDepth = depth
			for flag in preprocessorFlags:
				if line.find(flag) != -1:
					if preprocessorValues[count] == 0:
						visible = 0
				count = count + 1
		if line.find("#endif") != -1:
			if depth == flagDepth and visible == 0:
				visible = 1
			depth = depth - 1
		
		# Process block comment.  There may none, one or multiple comment blocks.
		if (line.find("/*") != -1):
	
			description = ''
			descriptionHTML = ''
			while line.find("*/") == -1:
				description = description + line.strip()
				descriptionHTML = descriptionHTML + line.strip()
				#only addd line break tag for HTML
				if descriptionHTML != '/*':
					descriptionHTML = descriptionHTML + "<br>"
				#outFileHTML.write("<p> %s<br></p>\n" % (re.sub('\A[^a-zA-Z0-9]*', '', line.strip())))
				line = inFile.readline()
			description = re.sub('\A[^a-zA-Z0-9]*', '', description) # strip out the leading non-alpha, non-numeric characters
			descriptionHTML = re.sub('\A[^a-zA-Z0-9]*', '', descriptionHTML) # strip out the leading non-alpha, non-numeric characters
			
		# Process BEHAVIOUR block
		if line.find("BEHAVIOUR(") != -1 and visible == 1:
	
			# Get behavour name
			start = line.find('(')+1
			end = line.find(')')
			name = line[start:end].strip()
			words = re.split('[()]', line)
			name = words[1]
	
			# HTML Behaviour Header
			outFileHTML.write("<p><A name=%s><h3><a href=\"#quicklink\">%s:</a></h3></a> %s<br></p>\n" % (name, name, descriptionHTML))
			outFileHTML.write("<p><table class=\"stat\" width = 100%>\n")
			
			# XML Behaviour Header
			outFileXML.write("\t\t<Item type=\"rage__NMBehavior\">\n")
			outFileXML.write("\t\t\t<name>%s</name>\n" % (name))
			if (len(description) > 150): # too big?
				description = "... (check docs)"
			outFileXML.write("\t\t\t<description>%s</description>\n" % description);
			outFileXML.write("\t\t\t<params>\n")
			
			# Parameters and feedback
			# Loop until we find the beginning of the scope
			while line.find('{') != -1:
				line = inFile.readline()
			line = inFile.readline()
			
			# Flag first parameter/feedback so we can print a little legend
			# This is a bit clunky
			firstParameter = 1
			firstFeedback = 1
			
			# Loop until we find the end of the scope
			toggle = 0
			while line.find('}') == -1:
				parameterLine = (line.find("PARAMETER(") != -1) or (line.find("PARAMETERV(") != -1) or (line.find("PARAMETERV0(") != -1)
				commentedOutParameterLine = (line.find("//PARAMETER(") != -1) or (line.find("//PARAMETERV(") != -1) or (line.find("//PARAMETERV0(") != -1)
				if parameterLine and (not commentedOutParameterLine):
					#print line
					#raw_input("Press enter to continue")
					if firstParameter == 1:
						# HTML Parameter Header
						outFileHTML.write("\t<tr>\n")
						outFileHTML.write("\t\t<td style=\"background-color: rgb(200, 200, 200);\" width = 20%><b>&nbsp;Parameter</b></td>\n")
						outFileHTML.write("\t\t<td style=\"background-color: rgb(200, 200, 200);\" width = 8%><b>&nbsp;Type</b></td>\n")
						outFileHTML.write("\t\t<td style=\"background-color: rgb(200, 200, 200);\" width = 8%><b>&nbsp;Default</b></td>\n")
						outFileHTML.write("\t\t<td style=\"background-color: rgb(200, 200, 200);\" width = 8%><b>&nbsp;Range</b></td>\n")
						outFileHTML.write("\t\t<td style=\"background-color: rgb(200, 200, 200);\"><b>&nbsp;Description</b></td>\n")
						outFileHTML.write("\t</tr>\n")
						# reset first parameter flag
						firstParameter = 0
						
					# Collect parameter info
					words = re.split('[,()]', line)
					comment = re.split(';', line)
					
					paramName = words[1].strip()
					
					if (line.find("PARAMETERV(") != -1):
						paramType = "vector3"
						paramDefault = "%s, %s, %s" % (words[2].strip(), words[3].strip(), words[4].strip())
						paramMin = words[5].strip()
						paramMax = words[6].strip()
					elif (line.find("PARAMETERV0(") != -1):
						paramType = "vector3"
						paramDefault = "0, 0, 0"
						paramMax = words[2].strip()
						paramMin = "-%s" % paramMax
					# special case for vectors because there are extra separators
					elif words[2].find("rage::Vector3") != -1:
						paramType = "vector3"
						paramDefault = "%s, %s, %s" % (words[3].strip(), words[4].strip(), words[5].strip())
						paramMin = words[8].strip()
						paramMax = words[9].strip()
					else:
						paramDefault = words[2].strip()
						paramType = words[3].strip()
						paramMin = words[4].strip()
						paramMax = words[5].strip()
					paramDescription = re.sub('<', ' LT ', comment[1]) #words[6 to len(words)]
					paramDescription = re.sub('>', ' GT ', paramDescription) #words[6 to len(words)]
					
					# special case for strings
					if paramType.find("char *") != -1:
						paramType = "string"
						
					paramDefault = re.sub('"', '', paramDefault)
					
					if paramType.find("float") != -1 or paramType.find("vector3") != -1:
						paramDefault = re.sub('f', '0', paramDefault)
						
					paramMin = re.sub('"', '', paramMin)
					paramMin = re.sub('f', '0', paramMin)
					if paramMin.find("_MAX") != -1 or paramMin.find("_MIN") != -1:
						paramMin = ''
						
					paramMax = re.sub('"', '', paramMax)
					paramMax = re.sub('f', '0', paramMax)
					if paramMax.find("_MAX") != -1 or paramMax.find("_MIN") != -1:
						paramMax = ''
						
					# bools and strings don't have range
					if (paramType.find("bool") != -1) or (paramType.find("string") != -1)or (paramType.find("void") != -1):
						paramMin = ''
						paramMax = ''
					
					# strip out the leading non-alpha, non-numeric characters
					paramDescription = re.sub('\A[^a-zA-Z0-9]*', '', paramDescription)
					paramDescription = paramDescription.strip()
					
					# Write HTML Parameter Entry
					outFileHTML.write("\t<tr>\n")
					outFileHTML.write("%s width = 20%%>&nbsp;%s</td>\n" % (background[toggle], paramName))
					outFileHTML.write("%s width = 8%%>&nbsp;%s</td>\n" % (background[toggle], paramType))
					outFileHTML.write("%s width = 8%%>&nbsp;%s</td>\n" % (background[toggle], paramDefault))
					outFileHTML.write("%s width = 8%%>&nbsp;%s ... %s</td>\n" % (background[toggle], paramMin, paramMax))
					outFileHTML.write("%s >&nbsp;%s</td>\n" % (background[toggle], paramDescription))
					outFileHTML.write("\t</tr>\n")
					# Write  XML Parameter Entry
					outFileXML.write("\t\t\t\t<Item type=\"rage__NMParam\">\n")
					outFileXML.write("\t\t\t\t\t<name>%s</name>\n" % (paramName))
					outFileXML.write("\t\t\t\t\t<type>%s</type>\n" % (paramType))
					outFileXML.write("\t\t\t\t\t<init>%s</init>\n" % (paramDefault))
					if paramMin != '':
						outFileXML.write("\t\t\t\t\t<min value=\"%s\"/>\n" % paramMin)
					if paramMax != '':
						outFileXML.write("\t\t\t\t\t<max value=\"%s\"/>\n" % paramMax)
					# Write HTML Parameter Close
					outFileXML.write("\t\t\t\t\t<description>%s</description>\n" % paramDescription)
					# Write XML Parameter Close
					outFileXML.write("\t\t\t\t</Item>\n")
					# Flip background colors
					toggle = 1 - toggle
					
				if line.find("FEEDBACK(") != -1:
					if firstFeedback == 1:				
						# HTML Feedback Header
						outFileHTML.write("<p><table class=\"stat\" width = 100%>\n")
						outFileHTML.write("\t<tr>\n")
						outFileHTML.write("\t\t<td style=\"background-color: rgb(220, 180, 30);\" width = 15%><b>&nbsp;Feedback Type</b></td>\n")
						outFileHTML.write("\t\t<td style=\"background-color: rgb(220, 180, 30);\" width = 20%><b>&nbsp;Feedback Name</b></td>\n")
						outFileHTML.write("\t\t<td style=\"background-color: rgb(220, 180, 30);\" ><b>&nbsp;Description</b></td>\n")
						outFileHTML.write("\t</tr>\n")
						firstFeedback = 0
					words = re.split('[,()]', line)
					comment = re.split(';', line)
					outFileHTML.write("\t<tr>\n")
					outFileHTML.write("%s width = 10%%>&nbsp;%s</td>\n" % (feedbackBackground[toggle], words[1]))
					behavioursUsed = words[2]
					if behavioursUsed.find("this") != -1:
						behavioursUsed = name
					outFileHTML.write("%s width = 10%%>&nbsp;%s</td>\n" % (feedbackBackground[toggle], behavioursUsed))
					description = re.sub('\A[^a-zA-Z0-9]*', '', comment[1]) #re.sub('\A[^a-zA-Z0-9]*', '', words[3]) 	# strip out the leading non-alpha, non-numeric characters
					outFileHTML.write("%s >&nbsp;%s</td>\n" % (background[toggle], description))
					outFileHTML.write("\t</tr>\n")
					toggle = 1 - toggle
				if line.find("FEEDBACKPARAM(") != -1:
					words = re.split('[,()]', line)
					comment = re.split(';', line)
					outFileHTML.write("\t<tr>\n")
					outFileHTML.write("%s width = 10%%>&nbsp;</td>\n" % (feedbackBackground[toggle]))
					feedbackName = words[1]
					outFileHTML.write("%s width = 10%%>&nbsp;%s</td>\n" % (feedbackBackground[toggle], feedbackName))
					description = re.sub('\A[^a-zA-Z0-9]*', '', comment[1]) #re.sub('\A[^a-zA-Z0-9]*', '', words[3]) 	# strip out the leading non-alpha, non-numeric characters
					outFileHTML.write("%s >&nbsp;argNo = %s, type = %s. %s</td>\n" % (background[toggle], words[2], words[3], description))
					outFileHTML.write("\t</tr>\n")
					toggle = 1 - toggle
				if line.find("FEEDBACKDESCR(") != -1:
					words = re.split('[,()]', line)
					comment = re.split(';', line)
					outFileHTML.write("\t<tr>\n")
					outFileHTML.write("%s width = 10%%>&nbsp;</td>\n" % (feedbackBackground[toggle]))
					feedbackName = words[1]
					outFileHTML.write("%s width = 10%%>&nbsp;%s</td>\n" % (feedbackBackground[toggle], feedbackName))
					description = re.sub('\A[^a-zA-Z0-9]*', '', comment[1]) #re.sub('\A[^a-zA-Z0-9]*', '', words[3]) 	# strip out the leading non-alpha, non-numeric characters
					outFileHTML.write("%s >&nbsp;argNo = %s, value = %s. name = %s: %s</td>\n" % (background[toggle], words[2], words[3], words[4], description))
					outFileHTML.write("\t</tr>\n")
					toggle = 1 - toggle
					
				line = inFile.readline()
			#HTML Behaviour Tail
			outFileHTML.write("</table>\n")
			outFileHTML.write("</p><br><HR><br>\n")
			# XML Behaviour Tail
			outFileXML.write("\t\t\t</params>\n")
			outFileXML.write("\t\t</Item>\n")
		
		line = inFile.readline()
	
	# HTML Tail
	outFileHTML.write("</body>\n")
	outFileHTML.write("</html>\n")
	
	# Manifest Tail
	outFileXML.write("\t</behaviors>\n")
	outFileXML.write("</rage__NMBehaviorPool>\n")
