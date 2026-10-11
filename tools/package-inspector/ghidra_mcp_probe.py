"""Bounded read-only MCP discovery against Ghidra's documented loopback endpoint.

No tool calls, program edits, imports, analysis jobs or Codex configuration edits.
"""
import argparse
import json
import pathlib
import urllib.request


def probe(port):
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
        with urllib.request.urlopen(call,timeout=5) as response:
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
    return {'status':'connected','endpoint':url,'initialize':initialized,
        'tools':tools,'programs':programs}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port',type=int,default=8080)
    parser.add_argument('--output',type=pathlib.Path,required=True)
    args=parser.parse_args()
    if not 1024<=args.port<=65535: parser.error('Invalid port')
    try: report=probe(args.port)
    except Exception as error: report={'status':'unavailable','error':str(error)}
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps({'status':report['status'],'error':report.get('error'),
        'tools':[t['name'] for t in report.get('tools',{}).get('tools',[])],
        'programs':report.get('programs'),'output':str(args.output)},indent=2))
    return 0 if report['status']=='connected' else 1


if __name__=='__main__': raise SystemExit(main())
