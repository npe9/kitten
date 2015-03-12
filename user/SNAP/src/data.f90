!-----------------------------------------------------------------------
!
! MODULE: data_module
!> @brief
!> This module contains the variables and setup subroutines for the mock
!> cross section data. It establishes the number of groups and constructs
!> the cross section arrays.
!
!-----------------------------------------------------------------------

MODULE data_module

  USE global_module, ONLY: i_knd, r_knd, zero

  USE sn_module, ONLY: cmom

  USE iso_c_binding

  IMPLICIT NONE

  PUBLIC

  SAVE
!_______________________________________________________________________
!
! Module Input Variables
!
! ng       - number of groups
! mat_opt  - material layout, 0/1/2=homogeneous/center/corner, with
!            two materials, and material 2 nowhere/center/corner
! src_opt  - source layout, 0/1/2=homogenous/src-center/src-corner=
!            source everywhere/center of problem/corner, strength=10.0
! scatp    - 0/1=no/yes print the full scattering matrix to file 'slgg'
!_______________________________________________________________________

  INTEGER(i_knd) :: ng=1, mat_opt=0, src_opt=0, scatp=0
!_______________________________________________________________________
!
! Run-time variables
!
! v(ng)         - mock velocity array
! nmat          - number of materials
! mat(nx,ny,nz) - material identifier array
!
! qi(nx,ny,nz,ng)             - fixed source array for src_opt<3
! qim(nang,nx,ny,nz,noct,ng)  - fixed source array for src_opt>=3
!
! sigt(nmat,ng)          - total interaction
! siga(nmat,ng)          - absorption
! sigs(nmat,ng)          - scattering, total
! slgg(nmat,nmom,ng,ng)  - scattering matrix, all moments/groups
! vdelt(ng)              - time-absorption coefficient
!_______________________________________________________________________

  INTEGER(i_knd) :: nmat=1

  INTEGER(i_knd), POINTER, DIMENSION(:,:,:) :: mat

  REAL(r_knd), POINTER, DIMENSION(:) :: v, vdelt

  REAL(r_knd), POINTER, DIMENSION(:,:) :: sigt, siga, sigs

  REAL(r_knd), POINTER, DIMENSION(:,:,:,:) :: qi, slgg

  REAL(r_knd), POINTER, DIMENSION(:,:,:,:,:,:) :: qim
 
  INTEGER(c_size_t) array_size

  type(c_ptr) :: cptr_in

  CONTAINS


  SUBROUTINE data_allocate ( nx, ny, nz, nmom, nang, noct, timedep,    &
    istat )

!-----------------------------------------------------------------------
!
! Allocate data_module arrays.
!
!-----------------------------------------------------------------------

    INTEGER(i_knd), INTENT(IN) :: nx, ny, nz, nmom, nang, noct, timedep

    INTEGER(i_knd), INTENT(INOUT) :: istat
!_______________________________________________________________________
!
!   Establish number of materials according to mat_opt
!_______________________________________________________________________

    IF ( mat_opt > 0 ) nmat = 2
!_______________________________________________________________________
!
!   Allocate velocities
!_______________________________________________________________________

    istat = 0

    CALL CREATE_SHARED(noct, cmom, nmat)
    
    IF ( timedep == 1 ) THEN
      array_size = ng
      CALL ALLOCATE_ARRAY(array_size, cptr_in, 0)
      ! so we are creating an apsace.
      ! and we are sharing it.
      !DO_SOMETHING ! allocate aspace here
      CALL C_F_POINTER(cptr_in, v, [ng])
    ELSE
      ALLOCATE( v(0), STAT=istat )
    END IF
    IF ( istat /= 0 ) RETURN

    v = zero
!_______________________________________________________________________
!
!   Allocate the material identifier array. ny and nz are 1 if not
!   2-D/3-D.
!_______________________________________________________________________

    array_size = nx*ny*nz
    CALL ALLOCATE_ARRAY(array_size, cptr_in, 0)
    CALL C_F_POINTER(cptr_in, mat, [nx,ny,nz])

    IF ( istat /= 0 ) RETURN

    mat = 1
!_______________________________________________________________________
!
!   Allocate the fixed source array. If src_opt < 3, allocate the qi
!   array, not the qim. Do the opposite (store the full angular copy) of
!   the source, qim, if src_opt>=3 (MMS). Allocate array not used to 0.
!   ny and nz are 1 if not 2-D/3-D.
!_______________________________________________________________________

    IF ( src_opt < 3 ) THEN
      array_size = nx*ny*nz*ng
      CALL ALLOCATE_ARRAY(array_size, cptr_in, 0)
      CALL C_F_POINTER(cptr_in, qi, [nx,ny,nz,ng])  
      ALLOCATE(qim(0,0,0,0,0,0), STAT=istat )
      IF ( istat /= 0 ) RETURN
      qi = zero
    ELSE
      array_size = nx*ny*nz*ng
      CALL ALLOCATE_ARRAY(array_size, cptr_in, 0)
      CALL C_F_POINTER(cptr_in, qim, [nx,ny,nz,noct,ng])
      array_size = nx*ny*nz*ng*noct
      CALL share_init(array_size, cptr_in, 0)
      CALL C_F_POINTER(cptr_in, qim, [nx,ny,nz,noct,ng])
      IF ( istat /= 0 ) RETURN
      qi = zero
      qim = zero
    END IF
!_______________________________________________________________________
!
!   Allocate mock cross sections
!_______________________________________________________________________
   ALLOCATE( sigt(nmat,ng), siga(nmat,ng), sigs(nmat,ng), slgg(nmat,nmom,ng,ng), STAT=istat ) 
   IF ( istat /= 0 ) RETURN
    sigt = zero
    siga = zero
    sigs = zero
    slgg = zero
!_______________________________________________________________________
!
!   Allocate the vdelt array
!_______________________________________________________________________

    ALLOCATE( vdelt(ng), STAT=istat )
    IF ( istat /= 0 ) RETURN

    vdelt = zero
!_______________________________________________________________________
!_______________________________________________________________________

  END SUBROUTINE data_allocate


  SUBROUTINE data_deallocate

!-----------------------------------------------------------------------
!
! Deallocate data_module arrays.
!
!-----------------------------------------------------------------------
!_______________________________________________________________________

    DEALLOCATE( v )
    DEALLOCATE( mat )
    DEALLOCATE( qi, qim )
    DEALLOCATE( sigt, siga, sigs )
    DEALLOCATE( slgg )
    DEALLOCATE( vdelt )
!_______________________________________________________________________
!_______________________________________________________________________

  END SUBROUTINE data_deallocate


END MODULE data_module
