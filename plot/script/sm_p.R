myggsave <- function(...) {
  ggplot2::ggsave(...)
  invisible()
}


if (T) {
        x1 <- read.table("../data/sm_p/dist_3_2.dat", header=TRUE)
        library(ggplot2)
        library(grid)
        library(extrafont)
        font_import()
        loadfonts()

        ggplot(x1, aes(x=nodeid, y=num, fill=typeid)) +
        scale_fill_manual(name=element_blank(),
                          values=c(a="#3ec97a", b="#ff9933"),
                          breaks=c("a", "b"),
                          labels=c("Receive","Send")) +
        scale_x_discrete(
                         breaks=c("a", "b", "c", "d", "e", "f", "g", "h", "i", "j"),
                         labels=c("0", "1", "2", "3", "4", "5", "6", "7", "8", "9")) +
        scale_y_continuous(expand=c(0,0),
                           limits=c(0,15),
                           breaks=c(0,3,6,9,12,15)) +
        guides(fill=guide_legend(nrow=1, byrow=FALSE, keywidth=0.6,keyheight=0.6)) +
        geom_bar(color="black", stat="identity", width=0.8, position=position_dodge()) +
        #geom_errorbar(aes(ymin=min, ymax=max),width=.2, position=position_dodge(.8)) +
        geom_text(aes(label = num), position = position_dodge(0.8), hjust=-0.2, size=4,angle=90) +
        ylab("Blocks") +
        xlab("Node") +
        theme_classic() +
        theme(axis.text.x = element_text(size=11)) +
        theme(axis.text.y = element_text(size=11)) +
        theme(legend.text = element_text(size=10)) +
        theme(axis.title.x = element_text(size=11)) +
        theme(axis.title.y = element_text(size=11)) +
        #theme(axis.title = element_text(size=18)) +
        #theme(legend.position=c(0.4,0.95), legend.direction="horizontal", legend.key=element_rect(color="grey", size=0.8))
        theme(legend.position="top", legend.key=element_rect(color="grey", size=0.8)) +
        ggtitle("Number of blocks receives or sends") +
        myggsave("../pdf/sm_p_dist_3_2.pdf", width=4.2, height=3,device=cairo_pdf)
}

if (T) {
        x1 <- read.table("../data/sm_p/receive_3_2.dat", header=TRUE)
        library(ggplot2)
        library(grid)
        library(extrafont)
        font_import()
        loadfonts()

        ggplot(x1, aes(x=nodeid, y=num, fill=typeid)) +
        scale_fill_manual(name=element_blank(),
                          values=c(a="#3ec97a", b="#ff9933"),
                          breaks=c("a", "b"),
                          labels=c("Data","Parity")) +
        scale_x_discrete(
                         breaks=c("a", "b", "c", "d", "e", "f", "g", "h", "i", "j"),
                         labels=c("0", "1", "2", "3", "4", "5", "6", "7", "8", "9")) +
        scale_y_continuous(expand=c(0,0),
                           limits=c(0,15),
                           breaks=c(0,3,6,9,12,15)) +
        guides(fill=guide_legend(nrow=1, byrow=FALSE, keywidth=0.6,keyheight=0.6)) +
        geom_bar(color="black", stat="identity", width=0.8, position=position_dodge()) +
        #geom_errorbar(aes(ymin=min, ymax=max),width=.2, position=position_dodge(.8)) +
        geom_text(aes(label = num), position = position_dodge(0.8), hjust=-0.2, size=4,angle=90) +
        ylab("Blocks") +
        xlab("Node") +
        theme_classic() +
        theme(axis.text.x = element_text(size=11)) +
        theme(axis.text.y = element_text(size=11)) +
        theme(legend.text = element_text(size=10)) +
        theme(axis.title.x = element_text(size=11)) +
        theme(axis.title.y = element_text(size=11)) +
        #theme(axis.title = element_text(size=18)) +
        #theme(legend.position=c(0.4,0.95), legend.direction="horizontal", legend.key=element_rect(color="grey", size=0.8))
        theme(legend.position="top", legend.key=element_rect(color="grey", size=0.8)) +
        ggtitle("Type of blocks receives") +
        myggsave("../pdf/sm_p_receive_3_2.pdf", width=4.2, height=3,device=cairo_pdf)
}

if (T) {
        x1 <- read.table("../data/sm_p/send_3_2.dat", header=TRUE)
        library(ggplot2)
        library(grid)
        library(extrafont)
        font_import()
        loadfonts()

        ggplot(x1, aes(x=nodeid, y=num, fill=typeid)) +
        scale_fill_manual(name=element_blank(),
                          values=c(a="#3ec97a", b="#ff9933"),
                          breaks=c("a", "b"),
                          labels=c("Data","Parity")) +
        scale_x_discrete(
                         breaks=c("a", "b", "c", "d", "e", "f", "g", "h", "i", "j"),
                         labels=c("0", "1", "2", "3", "4", "5", "6", "7", "8", "9")) +
        scale_y_continuous(expand=c(0,0),
                           limits=c(0,15),
                           breaks=c(0,3,6,9,12,15)) +
        guides(fill=guide_legend(nrow=1, byrow=FALSE, keywidth=0.6,keyheight=0.6)) +
        geom_bar(color="black", stat="identity", width=0.8, position=position_dodge()) +
        #geom_errorbar(aes(ymin=min, ymax=max),width=.2, position=position_dodge(.8)) +
        geom_text(aes(label = num), position = position_dodge(0.8), hjust=-0.2, size=4,angle=90) +
        ylab("Blocks") +
        xlab("Node") +
        theme_classic() +
        theme(axis.text.x = element_text(size=11)) +
        theme(axis.text.y = element_text(size=11)) +
        theme(legend.text = element_text(size=10)) +
        theme(axis.title.x = element_text(size=11)) +
        theme(axis.title.y = element_text(size=11)) +
        #theme(axis.title = element_text(size=18)) +
        #theme(legend.position=c(0.4,0.95), legend.direction="horizontal", legend.key=element_rect(color="grey", size=0.8))
        theme(legend.position="top", legend.key=element_rect(color="grey", size=0.8)) +
        ggtitle("Type of blocks sends") +
        myggsave("../pdf/sm_p_send_3_2.pdf", width=4.2, height=3,device=cairo_pdf)
}

# if (T) {
#         x1 <- read.table("../data/storage_3_2.dat", header=TRUE)
#         library(ggplot2)
#         library(grid)
#         library(extrafont)
#         font_import()
#         loadfonts()

#         ggplot(x1, aes(x=nodeid, y=num, fill=typeid)) +
#         scale_fill_manual(name=element_blank(),
#                           values=c(a="#3ec97a", b="#ff9933"),
#                           breaks=c("a", "b"),
#                           labels=c("Data","Parity")) +
#         scale_x_discrete(
#                          breaks=c("a", "b", "c", "d", "e", "f", "g", "h", "i", "j"),
#                          labels=c("0", "1", "2", "3", "4", "5", "6", "7", "8", "9")) +
#         scale_y_continuous(expand=c(0,0),
#                            limits=c(0,50),
#                            breaks=c(0,10,20,30,40,50)) +
#         guides(fill=guide_legend(nrow=1, byrow=FALSE, keywidth=0.6,keyheight=0.6)) +
#         geom_bar(color="black", stat="identity", width=0.8, position=position_dodge()) +
#         #geom_errorbar(aes(ymin=min, ymax=max),width=.2, position=position_dodge(.8)) +
#         geom_text(aes(label = num), position = position_dodge(0.8), hjust=-0.2, size=4,angle=90) +
#         ylab("Blocks") +
#         xlab("Node") +
#         theme_classic() +
#         theme(axis.text.x = element_text(size=11)) +
#         theme(axis.text.y = element_text(size=11)) +
#         theme(legend.text = element_text(size=10)) +
#         theme(axis.title.x = element_text(size=11)) +
#         theme(axis.title.y = element_text(size=11)) +
#         #theme(axis.title = element_text(size=18)) +
#         #theme(legend.position=c(0.4,0.95), legend.direction="horizontal", legend.key=element_rect(color="grey", size=0.8))
#         theme(legend.position="top", legend.key=element_rect(color="grey", size=0.8)) +
#         ggtitle("Type of blocks in each node") +
#         myggsave("../pdf/storage_3_2.pdf", width=4.2, height=3,device=cairo_pdf)
# }
