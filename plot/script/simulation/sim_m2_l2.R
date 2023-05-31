# sm_g tasks
 if (T) {
     x1<-read.table("../../data/simulation/230530/sim_m2_l2.dat", header=TRUE)
     library(ggplot2)
     library(grid)
     library(extrafont)
     font_import()
     loadfonts()
 
     ggplot(x1, aes(x=code_param_id, y=max_load, fill=method_id)) +
     scale_fill_manual(name=element_blank(),
                       breaks=c("a","b","c","d","e","f","g"),
                       labels=c("RDRE","RDPM","BWRE","BWPM","BTRE","BTPM","BT"),
                       #values=c(b='#3366ff',c='#33cccc',d='#999966')) +
                       values=c(a="#DBDFEA",b="#ACB1D6",c="#ACB1D6",d="#146C94",e="19A7CE",f="#AFD3E2",g="red")) +
     scale_x_discrete(breaks=c("a","b","c","d","e"),
                      labels=c("4", "6", "8", "12", "16")) +
     scale_y_continuous(expand = c(0,0),limits=c(0,1400), breaks=c(0,250,500,750,1000,1250)) +
     guides(fill=guide_legend(nrow=1, byrow=TRUE, keywidth=1,keyheight=1)) +
     geom_bar(color="black", stat="identity", width=0.8, position=position_dodge()) +
 	# geom_errorbar(aes(ymin=min, ymax=max),width=.2, position=position_dodge(.8)) +
     ylab("Max load") +
     xlab("k") +
     theme_classic() +
     theme(axis.text.x = element_text(size=12, angle=0, hjust=0.5, vjust=1, color="black")) +
     theme(axis.text.y = element_text(size=15, color="black")) +
     theme(legend.text = element_text(size=15)) +
     theme(axis.title.x = element_text(size=15)) +
     theme(axis.title.y = element_text(size=15)) +
     theme(legend.position=c(0.8,0.95), legend.direction="vertical", legend.key=element_blank())
     ggsave("../../pdf/simulation/230530/sim_m2_l2.pdf", width=4, height=2.7,device=cairo_pdf)
 }
