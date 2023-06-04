# sm_g tasks
 if (T) {
     x1<-read.table("../../../data/simulation/230603/sim_m4_l2.dat", header=TRUE)
     library(ggplot2)
     library(grid)
     library(extrafont)
     font_import()
     loadfonts()
 
     ggplot(x1, aes(x=code_param_id, y=max_load, fill=method_id)) +
     scale_fill_manual(name=element_blank(),
                       breaks=c("a","b","c","g","d","e","f"),
                       labels=c("RDRE","BWRE","BTRE","BT","RDPM","BWPM","BTPM"),
                       values=c(a="#19376D",b="#8294C4",c="#00C4FF",d="#83764F",e="#C38154",f="#FFB84C",g="red")) +
     scale_x_discrete(breaks=c("a","b","c","d","e"),
                      labels=c("4", "6", "8", "12", "16")) +
     scale_y_continuous(expand = c(0,0),limits=c(0,4100), breaks=c(0,1000,2000,3000,4000)) +
     guides(fill=guide_legend(nrow=2, byrow=TRUE, keywidth=0.8,keyheight=0.8)) +
    #  geom_text(aes(label = max_load), hjust=-2, vjust=-0.4, color = "black", size=2, angle=60, stat="identity") +
     geom_bar(color="black", stat="identity", width=0.8, position=position_dodge()) +
 	# geom_errorbar(aes(ymin=min, ymax=max),width=.2, position=position_dodge(.8)) +
     ylab("Max load (blocks)") +
     xlab("k") +
     theme_classic() +
     theme(axis.text.x = element_text(size=12, angle=0, hjust=0.5, vjust=1, color="black")) +
     theme(axis.text.y = element_text(size=15, color="black")) +
     theme(legend.text = element_text(size=15)) +
     theme(axis.title.x = element_text(size=15)) +
     theme(axis.title.y = element_text(size=15)) +
     theme(legend.text = element_text(size=10)) +
     theme(legend.background=element_rect(fill = alpha("white", 0.0))) + 
     theme(legend.position=c(0.5,0.95), legend.direction="vertical", legend.key=element_blank())
     ggsave("../../../pdf/simulation/230603/sim_m4_l2.pdf", width=4, height=2.7,device=cairo_pdf)
 }
