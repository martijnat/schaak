//usr/bin/gcc $0 -O3 -o $0.bin -lSDL2 && ./$0.bin;exit
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#define INF 999999

#define max(a,b) (a<b?b:a)
#define min(a,b) (a<b?a:b)

#if 0
#define debug printf
#else
#define debug(...)
#endif

#define errfun if(err!=0){printf("[%s]:%d err=%d\n",__func__,__LINE__,err);err=0;exit(1);}
#define inboard(x,y) (x>=0 && x<8 && y>=0 && y<8)
#define xymask(x,y) (inboard(x,y)?(1UL<<(8UL*(y)+(x))):0UL)
#define swap(type,x,y) {type swaptemp=x;x=y;y=x;}

/* Piece-square tables */
int eval_P[64] = {0,0,0,0,0,0,0,0,15,16,17,18,18,17,16,15,
                  14,15,16,17,17,16,15,14,13,14,15,16,16,15,14,13,
                  12,13,14,15,15,14,13,12,11,12,13,14,14,13,13,11,
                  10,11,12,13,13,12,11,10,0,0,0,0,0,0,0,0};

int eval_N[64] = {30,31,32,32,32,32,31,30,
                  31,31,33,34,34,33,31,31,
                  32,33,34,35,35,34,33,32,
                  32,34,35,36,36,35,34,32,
                  32,34,35,36,36,35,34,32,
                  32,33,34,35,35,34,33,32,
                  31,31,33,34,34,33,31,31,
                  30,31,32,32,32,32,31,30};

int eval_B[64]={30,30,30,30,30,30,30,30,
                30,30,30,30,30,30,30,30,
                30,30,30,30,30,30,30,30,
                30,30,30,30,30,30,30,30,
                30,30,30,30,30,30,30,30,
                30,30,30,30,30,30,30,30,
                30,30,30,30,30,30,30,30,
                30,30,30,30,30,30,30,30};

int eval_R[64]={50,50,50,50,50,50,50,50,
                50,50,50,50,50,50,50,50,
                50,50,50,50,50,50,50,50,
                50,50,50,50,50,50,50,50,
                50,50,50,50,50,50,50,50,
                50,50,50,50,50,50,50,50,
                50,50,50,50,50,50,50,50,
                50,50,50,50,50,50,50,50};

int eval_Q[64]={90,90,90,90,90,90,90,90,
                90,90,90,90,90,90,90,90,
                90,90,90,90,90,90,90,90,
                90,90,90,90,90,90,90,90,
                90,90,90,90,90,90,90,90,
                90,90,90,90,90,90,90,90,
                90,90,90,90,90,90,90,90,
                90,90,90,90,90,90,90,90};

int eval_K[64]={200,200,200,200,200,200,200,200,
                200,200,200,200,200,200,200,200,
                200,200,200,200,200,200,200,200,
                200,200,200,200,200,200,200,200,
                200,200,200,200,200,200,200,200,
                200,200,200,200,200,200,200,200,
                200,200,200,200,200,200,200,200,
                200,200,200,200,200,200,200,200};

#define u64 unsigned long int
#define i64   signed long int
#define u16 unsigned short

typedef struct {
    u64 p,k,q,r,b,n; // Black pieces
    u64 P,K,Q,R,B,N; // White pieces
    char cq:1,ck:1,cQ:1,cK:1,turn:1; // Castling and turn
    u64 en_passant;
    void * prev;
    int movecount;
} board;

typedef struct {
    unsigned char from;
    unsigned char to;
    unsigned char promote:2; // Promotion: bishop, knight, rook, queen
} move;

typedef struct {
    move m[4096];
    u64 size;
} moves;

int err = 0;

// Function declarations
void drawboard(board);
moves validmoves(board);
board apply(board, move);
move mflip(move);
board flip(board);
board cboard(board);
int eqboard(board, board);
u64 hboard(board);
int is_check(board);
int board_status(board);
u64 knight_attacks(u64);

int eval_minmax(board b)
{
  int score=0;
  int s=board_status(b);
  if(s==1) return  0;
  if(s==2) return  INF;
  if(s==3) return -INF;
  for(int i=0;i<64;i++)
    {
      u64 m=(1UL<<i);
      if(b.P&m) score+=eval_P[i];
      if(b.N&m) score+=eval_N[i];
      if(b.B&m) score+=eval_B[i];
      if(b.R&m) score+=eval_R[i];
      if(b.Q&m) score+=eval_Q[i];
      if(b.K&m) score+=eval_K[i];
      if(b.p&m) score-=eval_P[63-i];
      if(b.n&m) score-=eval_N[63-i];
      if(b.b&m) score-=eval_B[63-i];
      if(b.r&m) score-=eval_R[63-i];
      if(b.q&m) score-=eval_Q[63-i];
      if(b.k&m) score-=eval_K[63-i];
    }
  /* int my_moves=validmoves(b).size; */
  /* int opp_moves=validmoves(flip(b)).size; */
  /* int movescore = 1 + my_moves - opp_moves; */
  /* movescore/=10; */
  /* score+=movescore; */
  score+=(b.turn!=0?5:-5);;
  return score;
}


int eval_relative(board b)
{
  return (b.turn!=0?-1:1) *eval_minmax(b);
}

u64 knight_attacks(u64 knights) {
    u64 l1 = (knights >> 1) & 0x7f7f7f7f7f7f7f7f;
    u64 l2 = (knights >> 2) & 0x3f3f3f3f3f3f3f3f;
    u64 r1 = (knights << 1) & 0xfefefefefefefefe;
    u64 r2 = (knights << 2) & 0xfcfcfcfcfcfcfcfc;
    u64 h1 = l1 | r1;
    u64 h2 = l2 | r2;
    return (h1<<16) | (h1>>16) | (h2<<8) | (h2>>8);
}

moves validmoves(board b)
{
  errfun;
  moves r={0};
  r.size=0;
  /* generate pseudomoves */
  moves P={0};
  P.size=0;
  u64 own_pieces = (b.turn==0)?(b.p|b.k|b.q|b.r|b.b|b.n):(b.P|b.K|b.Q|b.R|b.B|b.N);
  for(int from=0;from<64;from++){
    u64 from_mask=1UL<<from;
    if(from_mask&(b.n|b.N)) { /* quick knight moves */
      int offsets[8]={-17,-15,-10,-6,6,10,15,17};
      for(int i=0;i<8;i++)
        if(from+offsets[i]>=0 && from+offsets[i]<64 &&
           !(own_pieces & (1UL<<(from+offsets[i]))))
          {
            P.m[P.size].from=from;
            P.m[P.size].promote=0;
            P.m[P.size].to=from+offsets[i];
            P.size++;
          }
    } else if(from_mask&(b.b|b.B)) { /* quick bishop moves */
      int offsets[4]={7,9,-7,-9};
      for(int i=0;i<4;i++)
        for(int len=1;len<8;len++)
          if(from+len*offsets[i]>=0 && from+len*offsets[i]<64 &&
             !(own_pieces & (1UL<<(from+len*offsets[i]))))
            {
              P.m[P.size].from=from;
              P.m[P.size].promote=0;
              P.m[P.size].to=from+len*offsets[i];
              P.size++;
            }
    } else if(from_mask&(b.r|b.R)) { /* quick rook moves */
      int offsets[4]={1,-1,8,-8};
      for(int i=0;i<4;i++)
        for(int len=1;len<8;len++)
          if(from+len*offsets[i]>=0 && from+len*offsets[i]<64 &&
             !(own_pieces & (1UL<<(from+len*offsets[i]))))
            {
              P.m[P.size].from=from;
              P.m[P.size].promote=0;
              P.m[P.size].to=from+len*offsets[i];
              P.size++;
            }
    } else if(from_mask&(b.q|b.Q)) { /* quick queen moves */
      int y=from/8;
      int x=from%8;
      for(int dx=-1;dx<=1;dx++)
        for(int dy=-1;dy<=1;dy++)
          if ((dx!=0)|(dy!=0))
            for(int l=1;l<8;l++)
              if(inboard(x+l*dx,y+l*dy))
                {
                  P.m[P.size].from=from;
                  P.m[P.size].to=(y+l*dy)*8+(x+l*dx);
                  P.m[P.size].promote=0;
                  P.size++;
                }
    } else if(from_mask&(b.k|b.K)) { /* quick king moves */
      int offsets[8]={-9,-8,-7,-1,1,7,8,9};
      for(int i=0;i<8;i++)
        if(from+offsets[i]>=0 && from+offsets[i]<64 &&
           !(own_pieces & (1UL<<(from+offsets[i]))))
          {
            P.m[P.size].from=from;
            P.m[P.size].promote=0;
            P.m[P.size].to=from+offsets[i];
            P.size++;
          }
    } else if(from_mask&(b.P)) {
      if((from/8)==6){
        int offsets[4]={-8,-16,-7,-9};
        for(int i=0;i<4;i++)
          if(from+offsets[i]>=0 && from+offsets[i]<64 &&
             !(own_pieces & (1UL<<(from+offsets[i])))) {
            P.m[P.size].from=from;
            P.m[P.size].promote=0;
            P.m[P.size].to=from+offsets[i];
            P.size++;
          }
      } else if ((from/8)>1){
        int offsets[3]={-8,-7,-9};
        for(int i=0;i<3;i++)
          if(from+offsets[i]>=0 && from+offsets[i]<64 &&
             !(own_pieces & (1UL<<(from+offsets[i])))) {
            P.m[P.size].from=from;
            P.m[P.size].promote=0;
            P.m[P.size].to=from+offsets[i];
            P.size++;
          }
      } else {
        int offsets[3]={-8,-7,-9};
        for(int i=0;i<3;i++)
          if(from+offsets[i]>=0 && from+offsets[i]<64 &&
             !(own_pieces & (1UL<<(from+offsets[i])))) {
            for(int promo=0;promo<4;promo++)
              {
                P.m[P.size].from=from;
                P.m[P.size].promote=promo;
                P.m[P.size].to=from+offsets[i];
                P.size++;
              }
          }
      }
    } else if(from_mask&(b.p)) {
      if((from/8)==1){
        int offsets[4]={8,16,7,9};
        for(int i=0;i<4;i++)
          if(from+offsets[i]>=0 && from+offsets[i]<64 &&
             !(own_pieces & (1UL<<(from+offsets[i])))) {
            P.m[P.size].from=from;
            P.m[P.size].promote=0;
            P.m[P.size].to=from+offsets[i];
            P.size++;
          }
      } else if((from/8)==6){
        int offsets[3]={8,7,9};
        for(int i=0;i<3;i++)
          if(from+offsets[i]>=0 && from+offsets[i]<64 &&
             !(own_pieces & (1UL<<(from+offsets[i])))) {
            for(int p=0;p<4;p++){
              P.m[P.size].from=from;
              P.m[P.size].promote=p;
              P.m[P.size].to=from+offsets[i];
              P.size++;
            }
          }
      } else {
        int offsets[3]={8,7,9};
        for(int i=0;i<3;i++)
          if(from+offsets[i]>=0 && from+offsets[i]<64 &&
             !(own_pieces & (1UL<<(from+offsets[i])))) {
            P.m[P.size].from=from;
            P.m[P.size].promote=3;
            P.m[P.size].to=from+offsets[i];
            P.size++;
          }
      }
    }
  }
  /* prune pseudomoves */
  for(int i=0;i<P.size;i++)
    {
      err=0;
      apply(b,P.m[i]);
      if(err==0){
        r.m[r.size]=P.m[i];
        r.size++;
      }
      err=0;
    }
  return r;
}

board cboard(board b1)
{
  board b2;
  b2.p=b1.p;
  b2.P=b1.P;
  b2.q=b1.q;
  b2.Q=b1.Q;
  b2.k=b1.k;
  b2.K=b1.K;
  b2.r=b1.r;
  b2.R=b1.R;
  b2.n=b1.n;
  b2.N=b1.N;
  b2.b=b1.b;
  b2.B=b1.B;
  b2.cq=b1.cq;
  b2.ck=b1.ck;
  b2.cQ=b1.cQ;
  b2.cK=b1.cK;
  b2.turn=b1.turn;
  b2.en_passant=b1.en_passant;
  b2.movecount=b1.movecount;
  b2.prev=b1.prev;
  return b2;
}

int eqboard(board b1,board b2)
{
  return (b2.p==b1.p && b2.P==b1.P && b2.q==b1.q &&
          b2.Q==b1.Q && b2.k==b1.k && b2.K==b1.K &&
          b2.r==b1.r && b2.R==b1.R && b2.n==b1.n &&
          b2.N==b1.N && b2.b==b1.b && b2.B==b1.B &&
          b2.cq==b1.cq && b2.ck==b1.ck &&
          b2.cQ==b1.cQ && b2.cK==b1.cK &&
          b2.turn==b1.turn &&
          b2.en_passant==b1.en_passant &&
          b2.movecount==b1.movecount);
}

#define flipped_row(x,n) (((x>>(8UL*n))&255UL)<<(56UL-8UL*n))
#define flipped_bitboard(x) flipped_row(x,0)|flipped_row(x,1)|flipped_row(x,2)|flipped_row(x,3)|flipped_row(x,4)|flipped_row(x,5)|flipped_row(x,6)|flipped_row(x,7)

move mflip(move m)
{
  move r;
  r.from = (m.from%8)+(7-(m.from/8))*8;
  r.to = (m.to%8)+(7-(m.to/8))*8;
  r.promote=m.promote;
  return r;
}

board flip(board b0)
{
  board b=cboard(b0);
  u64 t=0UL;
  t=flipped_bitboard(b.p);b.p=flipped_bitboard(b.P);b.P=t;
  t=flipped_bitboard(b.k);b.k=flipped_bitboard(b.K);b.K=t;
  t=flipped_bitboard(b.q);b.q=flipped_bitboard(b.Q);b.Q=t;
  t=flipped_bitboard(b.r);b.r=flipped_bitboard(b.R);b.R=t;
  t=flipped_bitboard(b.b);b.b=flipped_bitboard(b.B);b.B=t;
  t=flipped_bitboard(b.n);b.n=flipped_bitboard(b.N);b.N=t;
  b.en_passant=flipped_bitboard(b.en_passant);
  b.turn=~b.turn;
  t=b.cq;b.cq=b.cQ;b.cQ=t;
  t=b.ck;b.ck=b.cK;b.cK=t;
  return b;
}

int board_status(board b)
{
  /* 0 = normal */
  /* 1 = draw */
  /* 2 = checkmate, white wins */
  /* 3 = checkmate, black wins */
  if(b.movecount>=50) return 1;
  moves M=validmoves(b);

  if(M.size>0){
    if (!b.p&!b.b&!b.n&!b.q&!b.r&
        !b.P&!b.B&!b.N&!b.Q&!b.R)
      return 1; /* draw by insufficient material */
    return 0;       /* Normal */
  }
  if(is_check(flip(b))==0 && (is_check(b)==0)) return 1;       /* Stalemate */
  return (b.turn!=0)?3:2;            /* Checkmate */
}

u64 movemask_dir(int x,int y,int dx,int dy,u64 own_pieces,u64 other_pieces)
{
  int inside=1;
  u64 m=0UL;
  while(inside)
    {
      x+=dx;
      y+=dy;
      inside = (x>=0 && x<=7 && y>=0 && y<=7);
      if(inside)
        {
          u64 m2=1UL<<(8*y+x);
          if(m2&own_pieces)
            return m;
          if(m2&other_pieces)
            inside=0;
          m|=m2;
        }
    }
  return m;
}
#define rookmask(x,y,w,b) (movemask_dir(x,y,-1,0,w,b)|movemask_dir(x,y,1,0,w,b)|movemask_dir(x,y,0,1,w,b)|movemask_dir(x,y,0,-1,w,b))
#define bishmask(x,y,w,b) (movemask_dir(x,y,-1,-1,w,b)|movemask_dir(x,y,-1,1,w,b)|movemask_dir(x,y,1,-1,w,b)|movemask_dir(x,y,1,1,w,b))

int attacked_dxdy(board b,int x,int y,int dx,int dy,u64 attacker,u64 blocker)
{
  while(inboard(x+dx,y+dy))
    {
      x+=dx;y+=dy;
      if(xymask(x,y)&blocker)  return 0;
      if(xymask(x,y)&attacker) return 1;
    }
  return 0;
}

int is_check(board b){
  if(b.turn==1) return is_check(flip(b));
  /* check for check of white king  */
  /* get king position */
  int kx=0,ky=0;
  int x,y;
  for(y=0;y<8;y++)
    for(x=0;x<8;x++)
      if(b.K&(1UL<<(8*y+x)))
        kx=x,ky=y;
  x=kx;y=ky;
  /* check pawns */
  if(b.p&(xymask(x-1,y-1)|xymask(x+1,y-1)))
    return 1;
  /* check knights */
  if(knight_attacks(b.n)&b.K)
    return 1;
  /* check kings */
  if(b.k&(xymask(x-1,y)|xymask(x+1,y)|xymask(x,y-1)|xymask(x,y+1)|
          xymask(x-1,y-1)|xymask(x-1,y+1)|xymask(x+1,y-1)|xymask(x+1,y+1)))
    return 1;
  /* check bishops/queens */
  u64 attackers =  b.q|b.b;
  u64 blockers  = b.r|b.n|b.k|b.p|b.P|b.K|b.Q|b.R|b.B|b.N;
  if (attacked_dxdy(b,x,y, 1, 1,attackers,blockers)|
      attacked_dxdy(b,x,y, 1,-1,attackers,blockers)|
      attacked_dxdy(b,x,y,-1, 1,attackers,blockers)|
      attacked_dxdy(b,x,y,-1,-1,attackers,blockers))
    return 1;
  /* check rooks/queens */
  attackers =  b.q|b.r;
  blockers  =  b.b|b.n|b.k|b.p|b.P|b.K|b.Q|b.R|b.B|b.N;
  if (attacked_dxdy(b,x,y,0, 1,attackers,blockers)|
      attacked_dxdy(b,x,y,1, 0,attackers,blockers)|
      attacked_dxdy(b,x,y,0,-1,attackers,blockers)|
      attacked_dxdy(b,x,y,-1,0,attackers,blockers))
    return 1;

  return 0;
}

board apply(board b,move m)
{
  errfun;
  board r=cboard(b);
  r.prev=&b;
  u64 black_pieces = r.p|r.k|r.q|r.r|r.b|r.n;
  u64 white_pieces = r.P|r.K|r.Q|r.R|r.B|r.N;
  debug("move: %c%c->%c%c\n",'a'+(m.from%8),'8'-(m.from/8),'a'+(m.to%8),'8'-(m.to/8));
  u64 from_mask = 1UL<<m.from;
  u64 to_mask = 1UL<<m.to;
  u64 valid_to=0;
  int x=m.from%8;
  int y=m.from/8;
  u64 new_en_passant=0UL;
  if (r.turn!=0){
    /* white to move */
    if(0 == (white_pieces&from_mask)){debug("move.from invalid\n");err=1;return r;}
    if(0 != (white_pieces&to_mask))  {debug("move.to fail\n");err=2;return r;}
    if(r.P&from_mask){
      r.movecount=0;
      /* single move */
      if((~black_pieces&(from_mask>>8)))
        valid_to|=from_mask>>8;
      /* double move */
      if(y==6 && !((black_pieces|white_pieces)&(from_mask>>8)) &&
         !((black_pieces|white_pieces)&(from_mask>>16))){
        valid_to|=from_mask>>16;
        /* allow en passant on the next turn */
        new_en_passant=(from_mask>>8)|(from_mask>>16);
      }
      /* capture */
      if (x<7 && black_pieces&(from_mask>>7)) valid_to|=from_mask>>7;
      if (x>0 && black_pieces&(from_mask>>9)) valid_to|=from_mask>>9;
      /* en passant capture */
      if (y==3 && x<7&&((from_mask>>7)==to_mask)&&(r.en_passant&to_mask)){r.p&=~(from_mask<<1);valid_to|=from_mask>>7;}
      if (y==3 && x>0&&((from_mask>>9)==to_mask)&&(r.en_passant&to_mask)){r.p&=~(from_mask>>1);valid_to|=from_mask>>9;}
      valid_to&=~white_pieces;
      if(!(valid_to&to_mask)){debug("invalid P move\n");err=3;return r;}
      r.P&=~from_mask;
      r.P|=to_mask;
      /* promotion */
      if(to_mask & 255)
        {
          r.P&=~to_mask;
          debug("always promote to queen for now\n");
          if(m.promote==3)       r.Q|=to_mask;
          else if (m.promote==2) r.R|=to_mask;
          else if (m.promote==1) r.N|=to_mask;
          else                   r.B|=to_mask;
        }
    }  else if(r.N&from_mask){
      for(int dy=-2;dy<=2;dy++)
        for(int dx=-2;dx<=2;dx++){
          if((x+dx)>=0 && (x+dx)<8 &&
             (y+dy)>=0 && (y+dy)<8 &&
             (dx*dx+dy*dy)==5)
            valid_to|=(1UL<<((y+dy)*8+(x+dx)));
        }
      if(!(valid_to&to_mask)){debug("invalid N move\n");err=7;return r;}
      r.N&=~from_mask;
      r.N|=to_mask;
    } else if(r.B&from_mask){
      valid_to=bishmask(x,y,white_pieces,black_pieces);
      if(!(valid_to&to_mask)){debug("invalid B move\n");err=8;return r;}
      r.B&=~from_mask;
      r.B|=to_mask;
    } else if(r.R&from_mask){
      valid_to=rookmask(x,y,white_pieces,black_pieces);
      if(!(valid_to&to_mask)){debug("invalid R move\n");err=6;return r;}
      if(from_mask&0x8080808080808080) r.cK=0;
      if(from_mask&0x101010101010101)  r.cQ=0;
      r.R&=~from_mask;
      r.R|=to_mask;
    } else if(r.Q&from_mask){
      valid_to=rookmask(x,y,white_pieces,black_pieces);
      valid_to|=bishmask(x,y,white_pieces,black_pieces);
      if(!(valid_to&to_mask)){debug("invalid Q move\n");err=5;return r;}
      r.Q&=~from_mask;
      r.Q|=to_mask;
    } else if(r.K&from_mask){
      for(int dx=-1;dx<2;dx++)
        for(int dy=-1;dy<2;dy++){
          valid_to|=1UL<<((max(0L,min(7L,y+dy))*8L) + (max(0L,min(7L,x+dx))));
        }
      valid_to&=~white_pieces;
      int castle_err=0;
      /* O-O Short, king side castling */
      if (!is_check(r) && r.cK && (m.from==60) && (m.to==62)){
        r.cQ=0; r.cK=0;
        move cK1={60,61,0};
        move cK2={61,62,0};

        err=0;
        r=apply(r,cK1); castle_err|=err; r.turn=1;
        err=0;
        r=apply(r,cK2); castle_err|=err;
        err=castle_err;
        r.R&=~(1UL<<63);
        r.R|=(1UL<<61);
        return r;
      }

      /* O-O-O Long, queen side castle */
      if (!is_check(r) && r.cQ && (m.from==60) && (m.to==58)){
        r.cQ=0; r.cK=0;
        move cQ1={60,59,0};
        move cQ2={59,58,0};

        err=0;
        r=apply(r,cQ1); castle_err|=err; r.turn=1;
        err=0;
        r=apply(r,cQ2); castle_err|=err;
        err=castle_err;
        r.R&=~(1UL<<56);
        r.R|=(1UL<<59);
        return r;
      }

      if(!(valid_to&to_mask)){debug("invalid K move\n");err=4;return r;}
      /* remove castling rights on king move */
      r.cQ=0;
      r.cK=0;
      r.K&=~from_mask;
      r.K|=to_mask;
    } else {
      debug("Invalid state\n");err=9;return r;
    }
    /* captures */
    white_pieces = r.P|r.K|r.Q|r.R|r.B|r.N;
    if(white_pieces & black_pieces) r.movecount=0;
    r.p&=~white_pieces;
    r.k&=~white_pieces;
    r.q&=~white_pieces;
    r.r&=~white_pieces;
    r.b&=~white_pieces;
    r.n&=~white_pieces;

    r.en_passant=new_en_passant;
    r.turn=~r.turn;
  } else {
    /* black to move */
    b=flip(b);      /* 1. miror the board */
    m=mflip(m);     /* 2. mirror the move */
    b=apply(b,m);   /* 3. play the move as white */
    return flip(b); /* 4. re-flip the board */
  }
  /* final checks */
  /* forbid capture of kings */
  if(0UL==r.k || 0UL==r.K){err=10;return r;}
  if(is_check(r)!=0)      {err=11;return r;}
  r.movecount++;
  return r;
}



board startpos()
{
  board b;
  b.movecount=0;
  b.k=1UL<<4;
  b.q=1UL<<3;
  b.b=1UL<<2|1UL<<5;
  b.n=1UL<<1|1UL<<6;
  b.r=1UL<<0|1UL<<7;
  b.p=255UL<<8;

  b.K=1UL<<60;
  b.Q=1UL<<59;
  b.B=1UL<<58|1UL<<61;
  b.N=1UL<<57|1UL<<62;
  b.R=1UL<<56|1UL<<63;
  b.P=255UL<<48;

  b.cq=1,b.ck=1,b.cQ=1;b.cK=1;
  b.turn=1;
  b.en_passant=0UL;
  b.prev=NULL;

  return b;
}


void drawboard(board b)
{
  errfun;
  printf("\033[0m\n");
  for(int y=0;y<8;y++)
    {
      for(int x=0;x<8;x++)
        {
          char *c=((x+y)&1)?".":".";
          u64 mask = 1UL<<(y*8UL+x);
          if (mask&b.p) c="p";
          if (mask&b.q) c="q";
          if (mask&b.k) c="k";
          if (mask&b.n) c="n";
          if (mask&b.b) c="b";
          if (mask&b.r) c="r";
          if (mask&b.P) c="P";
          if (mask&b.Q) c="Q";
          if (mask&b.K) c="K";
          if (mask&b.N) c="N";
          if (mask&b.B) c="B";
          if (mask&b.R) c="R";
          printf(((x+y)&1)?"%s ":"%s ",c);
        }
      printf(" %c\n",'8'-y);
    }
  for(int x=0;x<8;x++)
    printf("%c ",'a'+x);
  printf(" %s%s%s%s%s\n",b.turn?"w":"b"
         ,b.cQ?"Q":"",b.cK?"K":""
         ,b.cq?"q":"",b.ck?"k":""
         );
  return;
}


move ai_random(board b)
{
  moves moveset=validmoves(b);
  move r={0};
  if(moveset.size<=0){
    err=1;
  }
  else{
    r=moveset.m[rand()%moveset.size];
  }
  return r;
}

move ai_simple(board b)
{
  moves moveset=validmoves(b);
  move bestmove=moveset.m[0];
  int besteval =eval_relative(apply(b,moveset.m[0]));
  for(int i=0;i<moveset.size;i++)
    {
      int eval=eval_relative(apply(b,moveset.m[i]));
      if(eval>besteval){
        bestmove=moveset.m[i];
        besteval=eval;
      }
    }
  return bestmove;
}

// Piece values for MVV-LVA, quick eval for move ordering
// (Most Valuable Victim - Least Valuable Aggressor)
int move_score_mmv_lva(board b, move m) {
  int piece_value[6] = {100, 300, 300, 500, 900, 2000}; // P, N, B, R, Q, K
    int score = 0;
    u64 to_mask = 1UL << m.to;

    // Capture bonus (MVV-LVA)
    int victim = -1;
    int aggressor = -1;
    u64 pieces[] = {b.P, b.N, b.B, b.R, b.Q, b.K, b.p, b.n, b.b, b.r, b.q, b.k};

    for (int i = 0; i < 12; i++) {
        if (pieces[i] & to_mask) {
            victim = i % 6;
            break;
        }
    }

    u64 from_mask = 1UL << m.from;
    for (int i = 0; i < 12; i++) {
        if (pieces[i] & from_mask) {
            aggressor = i % 6;
            break;
        }
    }

    if (victim >= 0) {
        score = 10000 + piece_value[victim] * 10 - piece_value[aggressor];
    }

    return score;
}

int eval_negamax(board b,int depth,int alpha,int beta)
{
  if(depth<=0)return -eval_relative(b);
  moves M=validmoves(b);
  if(M.size==0) return -eval_relative(b); /* stalemate or checkmate */
  int best=-INF;

  /* Score and sort moves quickly */
  int scores[M.size];
  for (int i = 0; i < M.size; i++) {
    scores[i] = move_score_mmv_lva(b, M.m[i]);
  }
  /* Simple bubble sort */
  for(int i=0;i<M.size;i++){
    for(int j=i+1;j<M.size;j++){
      if(scores[j-1]<scores[j]){
        swap(int,scores[j-1],scores[j]);
        swap(move,M.m[j-1],M.m[j]);
      }
    }
  }

  for(int i=0;i<M.size;i++) {
      int score= -eval_negamax(apply(b,M.m[i]),depth-1,-beta, -alpha);
      if (score >= beta) return beta;
      if (score > alpha) alpha = score;
    }
  return alpha;
}

move ai_negamax(board b,int depth)
{
  int pcount=0;
  moves moveset=validmoves(b);
  move bestmove={0};
  int besteval = -INF;
  int R=rand();
  for(int j=0;j<moveset.size;j++)
    {
      int i = (j+R)%moveset.size;
      i = (i+moveset.size)%moveset.size;
      move m=moveset.m[i];
      int eval=-eval_negamax(apply(b,m),depth,-INF,INF);
      if(eval>=besteval){
        besteval=eval;
        bestmove=m;
      }
      if(eval==INF) return bestmove; /* early exit for _any_ forced mate. */
    }
  return bestmove;
}
move ai_negamax1(board b) {return ai_negamax(b,1);}
move ai_negamax2(board b) {return ai_negamax(b,2);}
move ai_negamax3(board b) {return ai_negamax(b,3);}
move ai_negamaxx(board b) {return ai_negamax(b,4);} /* highest depth that can repond in a few seconds */



int tableA[6][64]={0,};
int tableB[6][64]={0,};
int eval_tablerel(board b,int table)
{
  int score=0;
  if(b.movecount>=50) return 0;
  int s=board_status(b);
  if(s==1) return  0;
  if(s==2) return  INF;
  if(s==3) return -INF;
  for(int i=0;i<64;i++)
    {
      u64 m=(1UL<<i);
      if((table%2)==0){
        if(b.P&m)score+=tableA[0][i];
        if(b.N&m)score+=tableA[1][i];
        if(b.B&m)score+=tableA[2][i];
        if(b.R&m)score+=tableA[3][i];
        if(b.Q&m)score+=tableA[4][i];
        if(b.K&m)score+=tableA[5][i];
        if(b.p&m)score-=tableA[0][64-i];
        if(b.n&m)score-=tableA[1][64-i];
        if(b.b&m)score-=tableA[2][64-i];
        if(b.r&m)score-=tableA[3][64-i];
        if(b.q&m)score-=tableA[4][64-i];
        if(b.k&m)score-=tableA[5][64-i];
      } else {
        if(b.P&m)score+=tableA[0][i];
        if(b.N&m)score+=tableA[1][i];
        if(b.B&m)score+=tableA[2][i];
        if(b.R&m)score+=tableA[3][i];
        if(b.Q&m)score+=tableA[4][i];
        if(b.K&m)score+=tableA[5][i];
        if(b.p&m)score-=tableA[0][64-i];
        if(b.n&m)score-=tableA[1][64-i];
        if(b.b&m)score-=tableA[2][64-i];
        if(b.r&m)score-=tableA[3][64-i];
        if(b.q&m)score-=tableA[4][64-i];
        if(b.k&m)score-=tableA[5][64-i];
      }
    }
  return (b.turn!=0?-1:1) * score;
}
int eval_table(board b,int depth,int alphabeta,int table)
{
  if(depth<=0) return -eval_tablerel(b,table);
  int max=-INF;
  moves M=validmoves(b);
  for(int i=0;i<M.size;i++)
    {
      int score= -eval_table(apply(b,M.m[i]),depth-1,-max,table);
      if(score>alphabeta)
        return score;
      if(score>max)
        max=score;
    }

  return max;
}
move ai_table(board b,int table)
{
  int pcount=0;
  int depth=1;
  moves moveset=validmoves(b);
  move bestmove={0};
  int besteval = -INF;
  int R=rand();
  for(int j=0;j<moveset.size;j++)
    {
      int i = (j+R)%moveset.size;
      i = (i+moveset.size)%moveset.size;
      move m=moveset.m[i];
      int eval=-eval_table(apply(b,m),depth,-besteval,table);
      if(eval>=besteval){
        besteval=eval;
        bestmove=m;
      }
    }
  return bestmove;
}
move ai_tableA(board b) {return ai_table(b,0);}
move ai_tableB(board b) {return ai_table(b,1);}


void input_cleanup(char *s)
{
  /* removes whitespace and upperspace from s */
  char t;
  int i=0;
  for(int j=0;s[j]!=0;j++)
    {
      if(s[j]>='A'&&s[j]<='Z') s[j]^='a'^'A';
      if((s[j]>='0'&&s[j]<='9') || (s[j]>='a'&&s[j]<='z'))
        {
          s[i]=s[j];
          i++;
        }
    }
  s[i]=0;
}

move player(board b)
{
  const int bufsize=512;
  move r;
  moves M=validmoves(b);
  move to[64];
  move K[64];
  move Q[64];
  move N[64];
  move B[64];
  move R[64];
  move P[64];
  int promo=3;
  /* printf("%d player moves\n",M.size); */
  for(int i=0;i<M.size;i++)
    {
      /* printf("move %c%c->%c%c\n",'a'+M.m[i].from%8,'1'+(7-M.m[i].from/8),'a'+M.m[i].to%8,'1'+(7-M.m[i].to/8)); */
      to[M.m[i].to]=M.m[i];
      if((1UL<<M.m[i].from)&(b.k|b.K)) K[M.m[i].to]=M.m[i];
      if((1UL<<M.m[i].from)&(b.q|b.Q)) Q[M.m[i].to]=M.m[i];
      if((1UL<<M.m[i].from)&(b.n|b.N)) N[M.m[i].to]=M.m[i];
      if((1UL<<M.m[i].from)&(b.b|b.B)) B[M.m[i].to]=M.m[i];
      if((1UL<<M.m[i].from)&(b.r|b.R)) R[M.m[i].to]=M.m[i];
      if((1UL<<M.m[i].from)&(b.p|b.P)) P[M.m[i].to]=M.m[i];
    }
  for(int i=0;i<M.size;i++) /* prioritize pawn moves from to coords */
    if((1UL<<M.m[i].from)&(b.p|b.P)) to[M.m[i].to]=M.m[i];
  while(1){
    drawboard(b);
    printf(b.turn?"White> ":"Black> ");
    char s[bufsize];
    char *e=fgets(s,bufsize,stdin);
    if(e==NULL){
      printf("\n");
      exit(1);
    }
    input_cleanup(s);
    int len=0;
    while(s[len]!=0)len++;
    if(s[len-1]=='q') promo=3,s[len-1]=0,len--;
    if(s[len-1]=='r') promo=2,s[len-1]=0,len--;
    if(s[len-1]=='n') promo=1,s[len-1]=0,len--;
    if(s[len-1]=='b') promo=0,s[len-1]=0,len--;
    if(len==4){
      {
        err=0;
        if(s[0]<'a'|s[0]>'h')err=1;
        if(s[1]<'1'|s[1]>'8')err=1;
        if(s[2]<'a'|s[2]>'h')err=1;
        if(s[3]<'1'|s[3]>'8')err=1;
        if(err==0){
          r.from = (s[0]-'a')+8*('8'-s[1]);
          r.to = (s[2]-'a')+8*('8'-s[3]);
          r.promote=promo;
          return r;
        }
        err=0;
      }
    } else if(len==3){
      /* long castle */
      if(s[0]=='o' && b.turn!=0){move castle={60,58,0}; return castle;}
      if(s[0]=='o' && b.turn==0){move castle={4 ,2 ,0}; return castle;}
      int ind=((s[1]-'a')+8*('8'-s[2]))%64;
      switch(s[0]){
      case 'p': return P[ind];
      case 'k': return K[ind];
      case 'q': return Q[ind];
      case 'r': return R[ind];
      case 'n': return N[ind];
      case 'b': return B[ind];
      }
    } else if(len==2) {
      /* short castle */
      if(s[0]=='o' && b.turn!=0){move castle={60,62,0}; return castle;}
      if(s[0]=='o' && b.turn==0){move castle={4 ,6 ,0}; return castle;}
      int ind=((s[0]-'a')+8*('8'-s[1]))%64;
      return to[ind];

    } else if(len==1) {
      /* short castle */
      if(s[0]=='x' && M.size>0){
        int r = rand()%M.size;
        return M.m[r];
      }
    }
  }
  printf("This should not be reached:%d\n",__LINE__);
  return r;
}


/* movers: */
/* - player */
/* - ai_random */
/* - ai_simple */

#define whitemove player
#define blackmove ai_negamax3
void playgame(){
  board b=startpos();
  move m={};
  int end=0;
  int turns=0;
  while(end==0 && b.movecount<50){
    drawboard(b);
    if(b.turn){
      m = whitemove(b);
    } else {
      m = blackmove(b);
    }
    printf("move %c%c->%c%c\n",'a'+m.from%8,'1'+(7-m.from/8),'a'+m.to%8,'1'+(7-m.to/8));

    err=0;
    board b2=apply(b,m);
    if(err==0){
      turns++;
      b=apply(b,m);
    } else
      err=0;
    end=board_status(b);
  }
  drawboard(b);
  printf("%d turns played\n",turns);
  if(b.movecount>=50) printf("\n[[ 50 move rule ]]\n");
  if(end==1) printf("\n[[ Draw ]]\n");
  if(end==2) printf("\n[[ White wins! ]]\n");
  if(end==3) printf("\n[[ Black wins! ]]\n");
  return;
}

#define test_ai1 ai_negamax3
#define test_ai2 ai_random
void testai(int ngames){
  int t0=time(NULL);
  int test1_win=0;
  int test2_win=0;
  int test_draw=0;
  for(int i=0;i<ngames;i++){
    board b=startpos();
    move m={};
    int end=0;
    int turns=0;
    int t1=time(NULL);
    int ai1_score=(test1_win*1000+test_draw*500)/ngames;
    while(end==0 && b.movecount<50){
      if((b.turn!=0)^(i%2)){
        m = test_ai1(b);
      } else {
        m = test_ai2(b);
      }
      err=0;
      board b2=apply(b,m);
      if(err==0){turns++; b=apply(b,m);}
      err=0;
      int eval=eval_minmax(b);
      end=board_status(b);
    }
    if(b.movecount>=50 || end==1) test_draw++;
    if(end==(2^(i%2))) test1_win++;
    if(end==(3^(i%2))) test2_win++;
    printf("|game %3i/%3i|ai1 win:%3d|ai2 win:%3d|draw: %3d|ai1 score:%4d|time = %i/%is\n",i+1,ngames,test1_win,test2_win,test_draw,ai1_score,(t1-t0),i>0?((ngames)*(t1-t0))/i:-1);
  }
  return;
}


void traintable(int ngames){
  int t0=time(NULL);
  printf("initializing table...\n");
  for(int i=0;i<64;i++){
    tableA[0][i]=100;
    tableA[1][i]=300;
    tableA[2][i]=300;
    tableA[3][i]=500;
    tableA[4][i]=900;
    tableA[5][i]=2000;
  }
  for(int i=0;i<ngames;i++){
    board b=startpos();
    move m={};
    int end=0;
    int turns=0;
    int t1=time(NULL);
    int totaltime=i>0?((ngames)*(t1-t0))/i:-1;
    printf("int eval_P[64]={");for(int i=0;i<64;i++) {printf("%d%s",tableA[0][i],i==63?"};\n":(i%8==7?",\n            ":","));};
    printf("int eval_N[64]={");for(int i=0;i<64;i++) {printf("%d%s",tableA[1][i],i==63?"};\n":(i%8==7?",\n            ":","));};
    printf("int eval_B[64]={");for(int i=0;i<64;i++) {printf("%d%s",tableA[2][i],i==63?"};\n":(i%8==7?",\n            ":","));};
    printf("int eval_R[64]={");for(int i=0;i<64;i++) {printf("%d%s",tableA[3][i],i==63?"};\n":(i%8==7?",\n            ":","));};
    printf("int eval_Q[64]={");for(int i=0;i<64;i++) {printf("%d%s",tableA[4][i],i==63?"};\n":(i%8==7?",\n            ":","));};
    printf("int eval_K[64]={");for(int i=0;i<64;i++) {printf("%d%s",tableA[5][i],i==63?"};\n":(i%8==7?",\n            ":","));};
    printf("|game %3i/%3i|time = %d/%ds\n",i+1,ngames,(t1-t0),totaltime>=0 && totaltime < 99999999?totaltime:-1);
    for(int i=0;i<64;i++)
      for(int j=0;j<6;j++)
        tableB[j][i]=tableA[j][i] -1 + (rand()%3);
    while(end==0 && b.movecount<50){
      int table_index=(b.turn!=0)^(i%2);
      m = ai_negamax1(b);
      /* printf("move %c%c->%c%c\n",'a'+m.from%8,'1'+(7-m.from/8),'a'+m.to%8,'1'+(7-m.to/8)); */
      err=0;
      board b2=apply(b,m);
      /* printf("err=%d\n",err); */
      if(err==0){b=apply(b,m);}
      err=0;
      /* drawboard(b); */
      end=board_status(b);
    }
    /* if(b.movecount>=50 || end==1) */
    if(end==(2^(i%2))){
      /* A wins */
      for(int i=0;i<64;i++)
        for(int j=0;j<6;j++)
          tableA[j][i]=tableA[j][i]+tableA[j][i]-tableB[j][i];
    }
    if(end==(3^(i%2))){
      /* B wins */
      for(int i=0;i<64;i++)
        for(int j=0;j<6;j++)
          tableA[j][i]=tableB[j][i];
    }
  }
  return;
}



int perft(board b,int depth)
{
  if(depth<=0) return 1;
  int r=0;
  moves M=validmoves(b);
  for(int i=0;i<M.size;i++)
    {
      err=0;
      r+=perft(apply(b,M.m[i]),depth-1);;
    }
  return r;
}

void selftest()
{
  u64 correct_perft[8]={1,20,400,8902,197281,4865609,119060324,3195901860};
  for(int i=0;i<8;i++){
    int t0=time(NULL);
    int result=perft(startpos(),i);
    int t1=time(NULL);
    printf("perft(%d)=%10d [%s] t=%ds\n",i,result,correct_perft[i]==result?" OK ":"FAIL",t1-t0);
  }
}

void randgame(int depth) {
  board b=startpos();
  printf("new game\n");
  for(int i=0;i<depth;i++)
    {
      move m=ai_random(b);
      printf("move %c%c->%c%c\n",'a'+m.from%8,'1'+(7-m.from/8),'a'+m.to%8,'1'+(7-m.to/8));
      b=apply(b,m);
    }
  printf("Game after %d random moves:\n",depth);
  drawboard(b);
}


void uci_game(){
  FILE * fp;
  fp = fopen("/home/erica/tinygames/schaak/log.txt", "w");
  const int bufsize=512;
  char s[bufsize];
  board b=startpos();
#define output(x) puts(x);fwrite("[OUTPUT]\n",10,1,fp);fwrite(x,sizeof(x),1,fp);fwrite("\n",1,1,fp);
  while(1){
    char *e=fgets(s,bufsize,stdin);
    if(e==NULL){exit(1);}
    int l=0;while(l<(bufsize-1) && s[l]!=0)l++;
    fwrite("[INPUT]\n",9,1,fp);
    fwrite(s,l,1,fp);
    if(memcmp(s,"uci\n",4)==0){
      output("id name schaak.c");
      output("id author Martijn Terpstra");
      output("uciok");
    } else if(memcmp(s,"isready\n",8)==0){
      output("readyok");
    } else if(memcmp(s,"ucinewgame\n",11)==0){
      b=startpos();
    } else if(memcmp(s,"position",8)==0){
      b=startpos(); /* ignore nondefault positions */
    } else {
      output("invalid command");
    }
  }
  fclose(fp);
}

int main(int argc, char ** argv)
{
  srand(time(NULL));
  /* uci_game(); */
  /* selftest(); */
  playgame();
  /* testai(100); */
  /* traintable(100); */
  /* randgame(4); */
  return 0;
}
