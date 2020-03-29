
import pymesh
import numpy as np
from scipy.sparse.linalg import inv as sp_inv
import scipy
import trimesh

class problem(object):
    
    def __init__(self,penalty,ex_force,mesh,nu_grad_size = 1e-8):
        
        self.mesh = mesh;
        assembler = pymesh.Assembler(mesh);
        self.init_M = assembler.assemble("lumped_mass");
        mesh.add_attribute("face_area")
        self.At0 = mesh.get_attribute("face_area")
        L = assembler.assemble("laplacian");
        hess_block = scipy.sparse.spmatrix.transpose(L)*sp_inv(self.init_M)* L;
        self.hess = scipy.sparse.block_diag((hess_block,hess_block,hess_block));
        self.laplacian = sp_inv(self.init_M)* L;
        self.penalty = penalty;
        self.nu_grad_size = nu_grad_size;
        self.x0 = np.reshape(self.mesh.vertices,np.size(self.mesh.vertices),order='F');
        self.pressure = ex_force;
        #######these are for the calculation of penalty hessian
        self.gradient_pre = 0;
        self.x_pre = np.zeros(np.size(self.mesh.vertices));
        self.hess_pre = np.eye(np.size(self.mesh.vertices));
    ###############################################################
               
    def obj_func_bending(self,x):
        
        a = np.asscalar(np.matrix(x)*self.hess.todense()*np.matrix(x).T/2);
        #print('bending =',a)
        return a
        
    def obj_func_penalty(self,x):
        
        shape_vertices = np.shape(self.mesh.vertices);
        vertices = np.reshape(x,shape_vertices,order='F');
        mesh = pymesh.form_mesh(vertices, self.mesh.faces);
        mesh.add_attribute("face_area")
        At = mesh.get_attribute("face_area")
        a = self.penalty*(np.linalg.norm((At - self.At0)/np.sqrt(self.At0)))**2
        
        # assembler = pymesh.Assembler(mesh);
        # M = assembler.assemble("lumped_mass");
        # a = self.penalty * scipy.sparse.linalg.norm(M-self.init_M,'fro'); # isometry 
        
        # conformal 
        # ratio = M[M.nonzero()]/self.init_M[self.init_M.nonzero()];
        # a = self.penalty * (np.var(ratio)) # penalty for conformalty # * (1/np.mean(ratio))**2); # penalty for isometry
        # print('mean =',np.mean(ratio))
        return a
        
    def obj_func(self,x):
        
        a = self.obj_func_bending(x) + self.obj_func_penalty(x);
        return a
    
    ######################################################################
 
    def gradient_bending(self,x):
        
        return self.hess * x
    
    def gradient_penalty(self,x):
        
        
        shape_vertices = np.shape(self.mesh.vertices);
        vertices = np.reshape(x,shape_vertices,order='F');
        mesh = pymesh.form_mesh(vertices, self.mesh.faces);
        mesh.add_attribute("face_normal")
        face_normal = mesh.get_attribute("face_normal")
        face_normal = np.reshape(face_normal,np.shape(mesh.faces))
        mesh.add_attribute("face_area")
        At = mesh.get_attribute("face_area")
        a = np.zeros(np.size(x))
        
        for v in range(shape_vertices[0]):
            mesh.enable_connectivity()
            adj_faces = mesh.get_face_adjacent_faces(v)
            av = 0
            for f in range(np.size(adj_faces)):
                tri_v = np.setdiff1d(mesh.faces[f],v)
                edge = vertices[tri_v[0]] - vertices[tri_v[1]]
                gradient = np.cross(face_normal[f],edge)
                
                if np.dot(gradient,vertices[v] - vertices[tri_v[0]])/(np.linalg.norm(gradient)*np.linalg.norm(vertices[v] - vertices[tri_v[0]]))<0:
                    gradient = - gradient
                    
                av = av + 2*gradient * (At[f] - self.At0[f])/self.At0[f] 
            a[3*v:3*v+3] = av
        #a = scipy.optimize.approx_fprime(x, self.obj_func_penalty,self.nu_grad_size);
        #print(a)
        return self.penalty*a
    
    def force(self,x):
        #mesh = trimesh.Trimesh(vertices=self.mesh.vertices;,
        #               faces=self.mesh.faces);
        shape_vertices = np.shape(self.mesh.vertices);
        vertices = np.reshape(x,shape_vertices,order='F');
        vector = self.laplacian*vertices;
        norm = np.linalg.norm(vector,2,1)
        norm = norm[:,None]
        vector = vector/norm;
        vector = np.reshape(vector,np.size(self.mesh.vertices),order='F');
        force = self.pressure*vector
        return force
        
        
        
    def gradient(self,x):
        a = self.gradient_bending(x)
        b = self.gradient_penalty(x) 
        c = self.force(x);
        print(np.linalg.norm(a),np.linalg.norm(b),np.linalg.norm(c));
        return a+b+c
    
    #############################################################################
    
    def hessian_bending(self):
        # hessian of bending is constant, could also call self.hess 
        a = self.hess.todense();
        #print('hess_bending',a);
        return a
    
    def hessian_penalty(self,x):
        grad = self.gradient_penalty(x)+self.force(x);
        s = x - (self.x_pre);
        y = grad - self.gradient_pre;
        C = np.outer(y,y)/np.inner(y,s);
        D = (self.hess_pre * np.outer(s,s) * self.hess_pre.T)\
            /np.asscalar(np.matrix(s)*self.hess_pre*np.matrix(s).T); 
        hess = self.hess_pre + C - D;
        self.gradient_pre = grad;
        self.x_pre = x;
        self.hess_pre = hess;
        #print('hessian_penalty',hess)
        return hess
        
    def hessian(self,x):
        #+ self.hessian_penalty(x)
        return self.hessian_bending() + self.hessian_penalty(x);
    
    
