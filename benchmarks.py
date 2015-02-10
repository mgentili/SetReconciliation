import argparse
import subprocess
import time
import json
import matplotlib.pyplot as plt
from itertools import groupby
filenames = {
			 'file_sync' : './bin/file_sync_testing',
			 'dir_sync'   : './bin/dir_sync_testing',
			 'gossip': './network_testing',
			}
tempdir = 'tmp/'
plotdir = 'plot/'

def getFileName( test_type, subtest_type, file_len, extra = "" ):
	return "{}{}_{}_{}_{}_{}.res".format( tempdir, test_type, subtest_type, file_len, extra, str(int(time.time())) )

def getBlockIncrement(start_block, end_block):
	#diff = end_block - start_block
	#return diff/num_trials
	return 10

def getBlockIncrementComplex(start_block, end_block, num_trials = 100):
	return (end_block - start_block)/num_trials

def generateRandData( file_len, start_block, end_block, error_prob, num_trials = 1):
	test_type = 'file_sync'
	subtest_type = 'rand'
	filename = getFileName( test_type, subtest_type, str(file_len), str(error_prob));
	fp = open(filename, 'w')
	for n in xrange(num_trials):
		for block_size in xrange(start_block, end_block, getBlockIncrement(start_block, end_block)):
			subprocess.call(
				[
					filenames[test_type], '--file-len', str(file_len),
					'--error-prob', str(error_prob), '--block-size', str(block_size), 
					'--rsync', "true"
				],
				stdout = fp
				)
	return filename

def generateBlockData( file_len, start_block, end_block, num_blocks, num_trials = 1):
	test_type = 'file_sync'
	subtest_type= 'block'
	filename = getFileName( test_type, subtest_type, str(file_len), str(num_blocks));
	fp = open(filename, 'w')
	for n in xrange(num_trials):
		for block_size in xrange(start_block, end_block, getBlockIncrement(start_block, end_block)):
			subprocess.call(
				[
					filenames[test_type], '--file-len', str(file_len),
					'--num-changes', str(num_blocks), '--block-size', str(block_size),
					'--rsync', "true" 
				],
				stdout = fp
				)
	return filename


def generateNetworkData( num_trials = 1):
	test_type = 'gossip'
	subtest_type='gossip'
	filename = getFileName( test_type, subtest_type, "", "");
	fp = open(filename, 'w')
	
	end_block = 1000
	for n in xrange(num_trials):
		subprocess.call(
			[
				filenames[test_type], '--l', str(file_len),
				'--b', str(num_blocks), '--avg', str(block_size) 
			],
			stdout = fp
		)
	return filename

def generateActualData( start_block, end_block, project, tag1, tag2, num_trials = 1):
	test_type = 'file_sync'
	subtest_type='actual'
	project = project.split('/')[1]
	filename = getFileName( test_type, subtest_type, project, "{}_{}".format(tag1, tag2) )
	fp = open(filename, 'w')
	for n in xrange(num_trials):
		for block_size in xrange(start_block, end_block, getBlockIncrementComplex(start_block, end_block)):
			subprocess.call( 
				[
					"./generate_similar_tag.sh", project, tag1, tag2
				]
			)
			subprocess.call(
				[
					filenames[test_type], '--f1', "A/{}".format(project),
					'--f2', "B/{}".format(project), '--block-size', str(block_size),
					 '--rsync', "true" 
				],
				stdout = fp
			)
	return filename
	
def parseJson( filename ):
	content = []
	with open(filename) as f:
	    for line in f:
	        while True:
		    #print len(content)
	            try:
	                jfile = json.loads(line)
	                break
	            except ValueError:
	            	try:
	            		line += next(f)
	            	except StopIteration:
	            		return content
	                # Not yet a complete JSON value
	        content.append( jfile )
	return content

def grouping(data, groupfunc, aggregatefunc):
	groups = []
	uniquekeys = []
	sorteddata = sorted( data, key=groupfunc )
	for k, g in groupby(sorteddata, groupfunc):
   		groups.append(list(g))    # Store group iterator as a list
   		uniquekeys.append(k)
   	ans = [ aggregatefunc(x) for x in groups]
   	return uniquekeys, ans

def generateGraph( info, xdata, ydata, labels):
	fig = plt.figure()
	for i in zip(xdata,ydata,labels):
		plt.plot(i[0],i[1], label=i[2])
	plt.xlabel(info['xlabel'])
	plt.ylabel(info['ylabel'])
	plt.ylim(ymin = 0, ymax=1.4*max(max( ydata, key=lambda r: max(r))))
	plt.legend(loc='upper left')
	fig.suptitle( info['title'] )
	fig.savefig( plotdir + info['filename'].replace(tempdir,"") + '.png')

def getMinBlockBytes( xvals, yvals ):
	return min( zip(xvals, yvals), key = lambda t: t[1] )

def createXYVals( content, start_block, end_block):
	params = ['total_bytes_no_strata', 'total_bytes_with_strata', 'rsync_bytes']
	captions = ["IBLT (no strata)", "IBLT (with strata)", "rsync"]
	#params = ['total_bytes_no_strata', 'total_bytes_with_strata', 'rsync_bytes', 'file2_size_compressed']
	#captions = ["IBLT (no strata)", "IBLT (with strata)", "rsync", "naive transfer (compressed)"]

	xs = []
	ys = []
	mins = []
	for p in params:
		x,y = grouping( content, lambda x: (int) (x['block_size']), lambda x: sum(r[p] for r in x)/len(x))
		x,y = zip(*filter( lambda x: x[0] >= start_block and x[0] <= end_block, zip(x,y)))
		xs.append(x)
		ys.append(y)
		mins.append(getMinBlockBytes(x, y))
	return xs, ys, mins, captions

def generateRandGraph( filename, start_block, end_block ):
	content = parseJson(filename)
	xs, ys, mins, captions = createXYVals( content, start_block, end_block )
	error_prob = content[0]['error_prob']
	file_len = content[0]['file_length']
	info = { 'xlabel' : 'block size(bytes)', 'ylabel' : 'bytes transferred', 'title' : "Random Error Model with p(err) = {}, File Length = {} bytes".format(error_prob, file_len) , 'filename' : filename}
	generateGraph( info, xs, ys, captions)
	print "{},".format(error_prob),
	for i in zip(captions, mins):
		print "{},{},".format(i[1][0], i[1][1]),
	print "{},".format(content[0]['file2_size_compressed']),
	print "{},{}".format(start_block,end_block)

def generateBlockGraph( filename, start_block, end_block ):
	content = parseJson(filename)
	xs, ys, mins, captions = createXYVals( content, start_block, end_block )
	num_blocks = content[0]['num_block_changes']
	file_len = content[0]['file_length']
	info = { 'xlabel' : 'block size(bytes)', 'ylabel' : 'bytes transferred', 'title' : "Block Error Model with {} blocks changed, File Length = {} bytes".format(num_blocks, file_len) , 'filename' : filename}
	generateGraph( info, xs, ys, captions)
	print "{},".format(num_blocks),
	for i in zip(captions, mins):
		print "{},{},".format(i[1][0], i[1][1]),
	print "{},".format(content[0]['file2_size_compressed']),
	print "{},{}".format(start_block,end_block)

def generateActualGraph( filename, start_block, end_block ):
	content = parseJson(filename)
	tags = filename.split("_")
	project = tags[-4]
	tag1 = tags[-3]
	tag2 = tags[-2]
	xs, ys, mins, captions = createXYVals( content, start_block, end_block )
	info = { 'xlabel' : 'block size(bytes)', 'ylabel' : 'bytes transferred', 'title' : "Transferring {} data from tag {} to tag {}".format(project, tag1, tag2) , 'filename' : filename}
	generateGraph( info, xs, ys, captions)
	
	print "{},".format(filename),
	for i in zip(captions, mins):
		print "{},{},".format(i[1][0], i[1][1]),
	print "{},".format(content[0]['file2_size_compressed']),
	print "{},{}".format(start_block,end_block),
	print "{}".format(abs(content[0]['file2_size'] - content[0]['file1_size']))

def generateNetworkGraph( filename ):
	content = parseJson(filename)
	caption = "multi-party IBLT"
	x,y = grouping( content, lambda x: (int) (x['num_distinct_keys']), lambda x: sum(r["num_rounds"] for r in x)/len(x))
	
	info = { 'xlabel' : 'Number of parties', 'ylabel' : 'Number of rounds to completion',  
		 'title' : "Number of rounds for all parties to receive a linear combination of all messages for p={}".format(filename), 'filename' : filename}
	generateGraph( info, xs, ys, caption)

def generateNetworkFailureGraph( filename ):
	content = parseJson(filename)
		
def main():
	parser = argparse.ArgumentParser(description='Generate file sync data and make graphs')
	parser.add_argument('-g', '--graphs', action='store_true', help='Whether to generate graphs')
	parser.add_argument('-r', '--rand', action='store_true')
	parser.add_argument('-b', '--block', action='store_true')
	parser.add_argument('-n', '--network', action='store_true')
	parser.add_argument('-a', '--actual', action='store_true')
	parser.add_argument('--file-len', default=1000000, type=int)
	parser.add_argument('--error-prob', default=0.001, type=float)
	parser.add_argument('--num-changes', default=5, type=int)
	parser.add_argument('--block-start', default= 10, type=int)
	parser.add_argument('--block-end', default= 100, type=int)
	parser.add_argument('--project', default='emacs')
	parser.add_argument('--tag1', default='emacs-24.1')
	parser.add_argument('--tag2', default='emacs-23.1')
	parser.add_argument('--file')
	args = vars(parser.parse_args())
	
	if args['graphs']:
		if args['rand']:
			generateRandGraph(args['file'], int(args['block_start']), int(args['block_end']))
		elif args['block']:
			generateBlockGraph(args['file'], int(args['block_start']), int(args['block_end']))
		elif args['actual']:
			generateActualGraph(args['file'], int(args['block_start']), int(args['block_end']))
	else:
		if( args['rand']):
			randData = generateRandData(args['file_len'], args['block_start'], args['block_end'], args['error_prob'])
			generateRandGraph(randData)
		if( args['block']):
			blockData = generateRandData(args['file_len'], args['block_start'], args['block_end'], args['num_changes'])
			generateBlockGraph(blockData)
		if( args['network'] ):
			networkData = generateNetworkData()
			generateNetworkGraph(networkData)
		if( args['actual'] ):
			actualData = generateActualData(args['block_start'], args['block_end'], args['project'], args['tag1'], args['tag2'])
			generateActualGraph(actualData, args['project'], args['tag1'], args['tag2'])

main()
