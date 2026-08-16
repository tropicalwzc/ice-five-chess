#!/usr/bin/env python3
"""Validate, summarize, and report the frozen five-star formal matches."""
import argparse, collections, hashlib, json, math, statistics
from pathlib import Path

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def load(paths):
    headers=[]; games=[]
    for p in paths:
        for line in p.read_text().splitlines():
            row=json.loads(line)
            (headers if row['type']=='header' else games).append(row)
    return headers,games
def five(board,x,y,s):
    for dx,dy in ((1,0),(0,1),(1,1),(1,-1)):
        n=1
        for sign in (-1,1):
            a,b=x+sign*dx,y+sign*dy
            while 0<=a<15 and 0<=b<15 and board[a][b]==s:
                n+=1;a+=sign*dx;b+=sign*dy
        if n>=5:return True
    return False
def validate(games):
    assert len(games)==200
    assert collections.Counter((g['openingId'],g['newColor']) for g in games)==collections.Counter((i,c) for i in range(100) for c in (1,-1))
    for g in games:
        assert g['anomaly'] is None
        b=[[0]*15 for _ in range(15)]; winner=0
        for x,y,s in g['moves']:
            assert winner==0 and 0<=x<15 and 0<=y<15 and b[x][y]==0
            b[x][y]=s
            if five(b,x,y,s):winner=s
        if g['termination']=='five-in-a-row': assert winner==g['winner']
        assert len(g['moves'])==g['moveCount']
def wilson(w,d,l):
    n=w+d+l;p=(w+.5*d)/n;z=1.95996398454;den=1+z*z/n
    center=(p+z*z/(2*n))/den
    half=z*math.sqrt(p*(1-p)/n+z*z/(4*n*n))/den
    return [center-half,center+half]
def stats(games,color=None):
    gs=[g for g in games if color is None or g['newColor']==color]
    w=sum(g['winner']==g['newColor'] for g in gs);d=sum(g['winner']==0 for g in gs);l=len(gs)-w-d
    return {'games':len(gs),'wins':w,'draws':d,'losses':l,'scoreRate':(w+.5*d)/len(gs),'wilson95':wilson(w,d,l)}
def report(name,opponent,games,headers,out):
    validate(games); white=stats(games,-1);black=stats(games,1);overall=stats(games)
    opponent_white_games=[dict(g,newColor=-g['newColor']) for g in games if g['newColor']==1]
    opponent_white=stats(opponent_white_games,-1)
    white_delta=white['scoreRate']-opponent_white['scoreRate']
    overall_delta=overall['scoreRate']-.5
    (out/f'{name}_raw.json').write_text(json.dumps({'headers':headers,'games':games},ensure_ascii=False,indent=2)+'\n')
    steps=[s for g in games for s in g['steps'] if s['engine']=='new']
    hits=[s for s in steps if s.get('corpusLookup')]
    changed=[s for s in hits if s.get('corpusAccepted') and (s['x'],s['y'])!=(s.get('fourStarX'),s.get('fourStarY'))]
    reasons=collections.Counter(str(s.get('corpusReason',0)) for s in steps)
    ms=sorted(s['ms'] for s in steps)
    summary={'schemaVersion':1,'opponent':opponent,'white':white,'opponentWhite':opponent_white,'whiteScoreDelta':white_delta,'black':black,'overall':overall,'overallScoreDelta':overall_delta,
      'corpus':{'lookups':len(hits),'accepted':sum(s.get('corpusAccepted',False) for s in hits),'changedMoves':len(changed),'fallbacks':len(steps)-len(changed),'reasons':dict(reasons)},
      'latencyMs':{'p50':statistics.median(ms),'p95':ms[min(len(ms)-1,math.ceil(.95*len(ms))-1)],'max':max(ms)},
      'anomalies':0,'validGames':len(games),'headers':headers}
    (out/f'{name}_summary.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2)+'\n')
    def line(label,s):
        a,b=s['wilson95'];return f"- {label}：{s['wins']}/{s['draws']}/{s['losses']}，得分率 {s['scoreRate']:.1%}，Wilson 95% {a:.1%}–{b:.1%}"
    text=f"""# 五星模型 vs {opponent} 正式对局报告

本报告按用户关注顺序先列五星执白。赛程为冻结后的 100 个未用于语料筛选或参数诊断的身份，每个身份交换颜色；共 200 局，重放验证全部合法，无遗漏异常。

## 结果

{line('五星执白',white)}
{line(opponent+'执白',opponent_white)}
- 白棋得分变化：{white_delta:+.1%}（五星执白减对手执白）
{line('五星执黑',black)}
{line('五星总体',overall)}

五子棋存在自然先手优势，报告不做颜色再加权。这里“五星执白”只统计 `newColor=-1` 的 100 局，不与对手执白混用。

## 语料与回退

- 五星决策 {len(steps)} 次；严格语料命中 {len(hits)} 次；通过接受门 {sum(s.get('corpusAccepted',False) for s in hits)} 次；相对四星实际改着 {len(changed)} 次。
- 回退或与四星同着 {len(steps)-len(changed)} 次。拒绝/状态码计数：`{json.dumps(dict(reasons),ensure_ascii=False,separators=(',',':'))}`。
- 四星对手完全不读取语料；旧三星对手使用冻结 `5224020` 实现。

## 性能与完整性

- 五星单步延迟 p50 {summary['latencyMs']['p50']:.1f} ms，p95 {summary['latencyMs']['p95']:.1f} ms，最大 {summary['latencyMs']['max']:.1f} ms。
- 200/200 局已逐手检查坐标、占用、终局和调度身份；anomaly=0。
- 评测采用 deterministic-best；用户对局中的随机只允许在证明、支持数与搜索评分等价的已接受候选之间。
"""
    (out/f'{name}.md').write_text(text)
    return summary
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--four',action='append',type=Path,required=True);ap.add_argument('--legacy',action='append',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=True)
    h4,g4=load(a.four);hl,gl=load(a.legacy);s4=report('five_star_vs_four_star','冻结无库四星',g4,h4,a.output);sl=report('five_star_vs_legacy_three_star','冻结旧三星',gl,hl,a.output)
    gate=(s4['whiteScoreDelta']>=0 and s4['overallScoreDelta']>=0 and sl['whiteScoreDelta']>=0 and sl['overallScoreDelta']>=0)
    comparison={'releaseGatePassed':gate,'classification':'eligible-safe-five-star' if gate else 'not-generalization-demonstrated','fiveVsFour':s4,'fiveVsLegacy':sl}
    (a.output/'five_star_comparison.json').write_text(json.dumps(comparison,ensure_ascii=False,indent=2)+'\n')
    (a.output/'five_star_comparison.md').write_text(f"# 五星双基线对比\n\n- 五星 vs 四星：五星执白 {s4['white']['scoreRate']:.1%}，四星执白 {s4['opponentWhite']['scoreRate']:.1%}，白棋变化 {s4['whiteScoreDelta']:+.1%}；总体变化 {s4['overallScoreDelta']:+.1%}\n- 五星 vs 旧三星：五星执白 {sl['white']['scoreRate']:.1%}，旧三星执白 {sl['opponentWhite']['scoreRate']:.1%}，白棋变化 {sl['whiteScoreDelta']:+.1%}；总体变化 {sl['overallScoreDelta']:+.1%}\n- 发布门：{'通过' if gate else '未通过'}；分类：`{comparison['classification']}`。\n\n发布门使用交换颜色后的相对变化：五星白棋得分减对手白棋得分，以及五星总体得分减 50%。这保留五子棋自然先手优势，不错误要求白棋原始胜率超过 50%。\n")
    files=sorted(p for p in a.output.iterdir() if p.is_file() and p.name!='checksums.sha256')
    (a.output/'checksums.sha256').write_text(''.join(f'{sha(p)}  {p.name}\n' for p in files))
if __name__=='__main__':main()
