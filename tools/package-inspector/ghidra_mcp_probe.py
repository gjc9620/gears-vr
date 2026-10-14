"""Bounded read-only MCP discovery against Ghidra's documented loopback endpoint.

Optional bounded read-only decompilation; no program edits, imports, analysis jobs
or Codex configuration edits.
"""
import argparse
import json
import pathlib
import urllib.request


def probe(port, inspect_addresses=(), program='GOWDE-Steam.exe', search_text=None, start_line=1):
    url = f'http://127.0.0.1:{port}/mcp'
    headers = {'Content-Type':'application/json','Accept':'application/json, text/event-stream'}
    counter = 0

    def request(method,params=None,notification=False):
        nonlocal counter
        counter += 1
        message = {'jsonrpc':'2.0','method':method}
        if params is not None: message['params']=params
        if not notification: message['id']=counter
        call = urllib.request.Request(url,json.dumps(message).encode(),headers,method='POST')
        with urllib.request.urlopen(call,timeout=35 if method=='tools/call' else 5) as response:
            session=response.headers.get('Mcp-Session-Id')
            if session: headers['Mcp-Session-Id']=session
            if notification: return None
            if 'text/event-stream' in response.headers.get('Content-Type',''):
                total=0
                while total<1024*1024:
                    line=response.readline(65536)
                    if not line: break
                    total+=len(line)
                    if line.startswith(b'data:'):
                        item=json.loads(line[5:].strip())
                        if item.get('id')==counter:
                            if 'error' in item: raise RuntimeError(item['error'])
                            return item.get('result')
                raise RuntimeError('No bounded MCP response')
            item=json.loads(response.read(1024*1024))
            if 'error' in item: raise RuntimeError(item['error'])
            return item.get('result')

    initialized=request('initialize',{'protocolVersion':'2025-03-26','capabilities':{},
        'clientInfo':{'name':'GearsVR read-only discovery','version':'1.0'}})
    headers['MCP-Protocol-Version']=initialized['protocolVersion']
    request('notifications/initialized',notification=True)
    tools=request('tools/list')
    programs=request('resources/read',{'uri':'ghidra://programs'})
    inspections=[]
    blocks=None
    if inspect_addresses:
        available={item['name']:item for item in tools.get('tools',[])}
        if 'inspect' not in available: raise RuntimeError('Server has no inspect tool')
        blocks=request('tools/call',{'name':'memory','arguments':{
            'file_name':program,'action':'list_blocks','page_size':20}})
        for address in inspect_addresses:
            options={'search_text':search_text,'context_lines':3,'max_matches':10} if search_text else {}
            result=request('tools/call',{'name':'inspect','arguments':{
                'file_name':program,'action':'decompile','address':address,
                'timeout':25,'max_lines':200,'start_line':start_line,**options}})
            entry={'address':address,'result':result}
            # Read even when decompilation succeeds: target identity is checked
            # against the validated snapshot rather than inferred from a name.
            entry['bytes']=request('tools/call',{'name':'memory','arguments':{
                'file_name':program,'action':'read','address':address,
                'length':64,'format':'hex'}})
            inspections.append(entry)
    return {'status':'connected','endpoint':url,'initialize':initialized,
        'tools':tools,'programs':programs,'inspected_program':program,
        'blocks':blocks,'inspections':inspections}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port',type=int,default=8080)
    parser.add_argument('--output',type=pathlib.Path,required=True)
    parser.add_argument('--program',default='GOWDE-Steam.exe',
        help='Exact open program name returned by ghidra://programs')
    parser.add_argument('--search-text',help='Bounded literal search within each decompiled function')
    parser.add_argument('--start-line',type=int,default=1,help='One-based first decompiled line; returns at most 200')
    parser.add_argument('--inspect-address',action='append',default=[],
        help='Read-only decompile and 64-byte check of a hex virtual address (max 4)')
    args=parser.parse_args()
    if not 1024<=args.port<=65535: parser.error('Invalid port')
    if len(args.inspect_address)>4: parser.error('At most four addresses per request')
    if args.start_line<1: parser.error('start-line must be positive')
    try:
        for address in args.inspect_address: int(address,16)
    except ValueError: parser.error('Addresses must be hexadecimal')
    try: report=probe(args.port,args.inspect_address,args.program,args.search_text,args.start_line)
    except Exception as error: report={'status':'unavailable','error':str(error)}
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps({'status':report['status'],'error':report.get('error'),
        'tools':[t['name'] for t in report.get('tools',{}).get('tools',[])],
        'programs':report.get('programs'),'output':str(args.output)},indent=2))
    for entry in report.get('inspections',[]):
        print(json.dumps({'address':entry['address'],
            'decompile_error':entry['result'].get('isError',False)}))
    return 0 if report['status']=='connected' else 1


if __name__=='__main__': raise SystemExit(main())
