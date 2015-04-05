#include<stdio.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<string.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<unistd.h>
#include<poll.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<limits.h>


struct nsd{
	int fd;    // node nsfd
	int dis;   // distance from server	
	int process_time;   // time for processing	
	int rackno;
	int threshold;
	int buf_size;
}nsdarr[7];


int cost[17][17],cap[17][17];

// cap[][]-capacity adjacency matrix
// cost[][]-cost per unit of flow matrix


// flow network and adjacency list
int fnet[17][17], adj[17][17], deg[17];

// Dijkstra's successor and depth
int par[17], d[17];        // par[source] = source;

// Labelling function
int pi[17];

#define CLR(a, x) memset( a, x, sizeof( a ) )
#define Inf (INT_MAX/2)

// Dijkstra's using non-negative edge weights (cost + potential)
#define Pot(u,v) (d[u]) // + pi[u] - pi[v]) 

int dijkstra( int n, int s, int t )
{
	int i;
    for( i = 0; i < n; i++ ) { d[i] = Inf; par[i] = -1;}
    d[s] = 0;
    par[s] = -n - 1;

    while( 1 ) 
    {
        // find u with smallest d[u]
        int u = -1, bestD = Inf;
        for( i = 0; i < n; i++ ) if( par[i] < 0 && d[i] < bestD )
            bestD = d[u = i];
        if( bestD == Inf ) break; //node cannot be reached

        // relax edge (u,i) or (i,u) for all i;
        par[u] = -par[u] - 1; //parent becomes positive once visited
        
        for( i = 0; i < deg[u]; i++ )
        {
            // try undoing edge v->u      
            int v = adj[u][i];
            if( par[v] >= 0 )  //if parent >0 already visited
            	continue;
            
            if(u==t) break; //sink

            if( fnet[v][u] && d[v] > Pot(u,v) - cost[v][u] ) 
                { d[v] = Pot( u, v ) - cost[v][u], par[v] = -u-1;}
        
            // try edge u->v
            if( fnet[u][v] < cap[u][v] && d[v] > Pot(u,v) + cost[u][v] ) 
                d[v] = Pot(u,v) + cost[u][v], par[v] = -u - 1;
        }
    }
  
      /* for( i = 0; i < n; i++ ) 
        if( pi[i] < Inf )
        {  
        pi[i] += d[i]; //potential based on distance
        }
      */
    return par[t] >= 0; //if sink has parent for this dijkstra's flow
}
#undef Pot

int mcmf( int n, int s, int t ,int * fcost)
{
    // build the adjacency list
    CLR( deg, 0 );
    int i,j;
    for(  i = 0; i < n; i++ )
    for(  j = 0; j < n; j++ ) 
        if( cap[i][j] || cap[j][i] ) adj[i][deg[i]++] = j;
        
    CLR( fnet, 0 );
    CLR( pi, 0 );
      int flow =0;
      *fcost = 0;
    
    // repeatedly, find a cheapest path from s to t
    while( dijkstra( n, s, t ) ) 
    {
        // get the bottleneck capacity

        int bot = INT_MAX;
        int v, u;

        for(  v = t, u = par[v]; v != s; u = par[v = u] )
            { 
            //if any path has bottleneck capacity 0 then djikstra wont find any more paths
               if(cap[u][v]-fnet[u][v]==0)return flow; 
           	   
               bot = (bot<= (cap[u][v]- fnet[u][v])) ? bot : ( cap[u][v] - fnet[u][v] );
           	}

        // update the flow network
        for(  v = t, u = par[v]; v != s; u = par[v = u] )
            if( fnet[v][u] ) 
            	{ 
            		fnet[v][u] -= bot; 
            		*fcost -= bot * cost[v][u]; 
            	}
            else 
            	{ 
            		fnet[u][v] += bot; 
                //printf(" \n %d %d %d \n",u,v, fnet[u][v]);

            		*fcost += bot * cost[u][v];
            	}
    
        flow += bot;
    }
  
    return flow;
}

int main()
{
	
    	int sd[7],i,j; //sd used for socket sfds

		for(i=0;i<16;i++)
		for(j=0;j<16;j++)
		{ cost[i][j]=-1; cap[i][j]=0;} // cost and capacity matrices for network flow graph

        
        struct sockaddr_in ser[7]; // ser- sockaddr structures for sockets
       	int port_no[]= {3127, 3129, 3130, 3131,3132, 3133, 3134}; //ports for clients


      
     	for( i=0; i<7; i++)
	  	{	
	  		sd[i]=socket(AF_INET,SOCK_STREAM,0);
        	perror("socket");
        
       		ser[i].sin_family=AF_INET;
    		ser[i].sin_port=htons(port_no[i]);
    		ser[i].sin_addr.s_addr=htonl(INADDR_ANY);
    		bind(sd[i],(struct sockaddr *)&ser[i],sizeof(ser[i]));
			  perror("bind");
			  listen(sd[i],5);
      	}
    
    
	int k=0;
	int dis[]={2,3,3,4,4,2,2}; //time due to distance array
	int speed[]={3,1,6,4,5,6,2}; //time taken for processing array
	int rack[]={1,2,2,3,3,4,4}; //rack array
	int buf[]={2,2,2,2,2,2,2}; // buffer on particular node

	struct pollfd fds[7];

	for(i=0;i<7;i++){
		nsdarr[i].dis=dis[i];
		nsdarr[i].process_time=speed[i];
		nsdarr[i].rackno=rack[i];
		nsdarr[i].threshold=2*(nsdarr[i].dis + nsdarr[i].process_time);
		nsdarr[i].buf_size=buf[i];
	}

    	while(k<7)
    	{
		    nsdarr[k].fd = accept(sd[k], NULL, NULL);
		    fds[k].fd=nsdarr[k].fd;
		    fds[k].events=POLLIN;
		    perror("accept");
		    printf("\nclient %d connected\n",k+1);
		    k++;
     	}

     	
     		sleep(15);
     		poll(fds,7,-1);
     		int requests[7]={0,0,0,0,0,0,0};
     		int count_of_requests=0;

			int edges=0;
     		
     		for(i=0; i<7 ;i++){
     			if(fds[i].revents ==POLLIN){ 
				requests[i]=1;
			 	count_of_requests++; 
			   }	 
     		}

     		int qualified[7]={0};

     		printf("%d requests \n",count_of_requests);

		
		//constructing cost and capacity matrices for passing to mcmf
		for(i=0;i<7;i++){

		   if(requests[i]){
		   		edges++;
		      for(j=0;j<7;j++){
			   if(i!=j){
				if(nsdarr[i].rackno==nsdarr[j].rackno)
				continue;
				if(nsdarr[i].threshold<(nsdarr[i].dis + nsdarr[i].process_time))
				continue;

				if(qualified[j]==0)
					{
						qualified[j]=1;
						edges++;
					}

				edges++;
				cap[0][i+1]=2; //capacity from source to request nodes= copies to be made
				cost[0][i+1]=1; 

				//cost for renundancy
				cost[i+1][j+8]=(nsdarr[i].dis + nsdarr[j].process_time + nsdarr[j].dis);
 				cap[i+1][j+8]=1;
				
				cost[j+8][15]=1;
				//capacity from qualified nodes to sink= buffer size of qualified nodes
				cap[j+8][15]=nsdarr[i].buf_size;

			   }
		       }
		    }
		 }
		 printf("%d total edges in graph \n", edges);

     int * fcost =malloc(sizeof(int));
		 int flow =mcmf(16, 0, 15, fcost );

     printf(" \n %d qualified flow from network has cost %d \n", flow, *fcost);

     //nodes which cannot be satisfied by qualified nodes
     int count=0;
     int qual_buf_left[7]={0};

		 for( i=0; i<7; i++)
		 	{
        if(requests[i]==0)continue;

        count=0;
         for( j=0; j<7; j++)
		 	    {
		 		
		 		     if(fnet[i+1][j+8]==1)
		 			  { 
              count++;
              qual_buf_left[j]++;
		 			    printf("\n data  from %d replicated to %d \n", i+1,j+1);
		 			  }
		 	    }

          if(count==2)requests[i]=0;
          else if(count==1) requests[i]=1;
          else requests[i]=2;
      }
			
    

      for(i=0; i<7;i++)
      {
        nsdarr[i].buf_size= nsdarr[i].buf_size- qual_buf_left[i];
        //printf("\n %d \n", nsdarr[i].buf_size);
      }

      edges=0;

      for(i=0;i<16;i++)
      for(j=0;j<16;j++)
       { 
         cost[i][j]=-1; 
          cap[i][j]=0;
       } 

    int unqualified[7]={0};

		  //creating matrix for unqualified nodes
      //constructing cost and capacity matrices for passing to mcmf
    for(i=0;i<7;i++)
  {
    if(requests[i])
    {
      edges++;
      for(j=0;j<7;j++)
      {
         if(i!=j)
         {
            if((nsdarr[i].rackno==nsdarr[j].rackno)||(nsdarr[i].threshold<(nsdarr[i].dis + nsdarr[i].process_time)))
           {
              if(unqualified[j]==0)
              {
                unqualified[j]=1;
                edges++;
              }

          edges++;
          cap[0][i+1]=requests[i]; 
          //capacity from source to request nodes= copies left to be made
          cost[0][i+1]=1; 

          //cost for renundancy
          cost[i+1][j+8]=(nsdarr[i].dis + nsdarr[j].process_time + nsdarr[j].dis);
          cap[i+1][j+8]=1;
        
          cost[j+8][15]=1;
          //capacity from qualified nodes to sink= buffer size of qualified nodes
          cap[j+8][15]=nsdarr[i].buf_size;

            } else continue;
          }
      }    
    }	
  }

  flow =mcmf(16, 0, 15, fcost);
  printf(" \n %d unqualified flow from network has cost %d \n", flow, *fcost);

  for( i=0; i<7; i++)
      {
        if(!requests[i])continue;
         for( j=0; j<7; j++)
          {
             if(fnet[i+1][j+8]==1 )
            { 
              
              printf("\n data  from %d replicated to unqualified node %d \n", i+1,j+1);
            }
          }
      }

    return 0;
}
