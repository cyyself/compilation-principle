#!/usr/bin/env python3
import ete3,json

def read_json():
	s = ""
	read_start = False
	while True:
		try:
			line = input()
			if line == "------ GRAMMAR ANALYZED TREE BEGIN -----":
				read_start = True
			elif line == "------ GRAMMAR ANALYZED TREE  END  -----":
				read_start = False
			else:
				if read_start:
					s += line.strip()
		except:
			break
	return json.loads(s)

def build_tree(jsonNode,tr=ete3.Tree(),depth = 0):
	if type(jsonNode) is dict:
		for key,value in jsonNode.items():
			this_node = tr.add_child(name=str(jsonNode))
			face = ete3.TextFace(key)
			this_node.add_face(face,column=1,position="branch-top")
			for child in value:
				build_tree(child,this_node,depth+1)
	else:
		tr.add_child(name=str(jsonNode))
	return tr

def print_tree(tr):
	style = ete3.TreeStyle()
	style.show_leaf_name = True
	tr.show(tree_style=style)

print_tree(build_tree(read_json()))
